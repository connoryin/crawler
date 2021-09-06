#include <algorithm>
#include <cmath>

#include "core/file_system.h"
#include "core/time.h"
#include "crawler/crawler.h"
#include "distributed/distributed.h"

Crawler::Crawler( const Vector<Url> &seedList, const CrawlerConfiguration &config ) : Crawler( config )
   { for ( const auto &url : seedList ) _frontier.emplace( url ); }

Crawler::Crawler( StringView checkpointFilePath, const CrawlerConfiguration &config ) : Crawler( config )
   {
   const auto beginTime = std::chrono::steady_clock::now( );
   std::cout << putCurrentDateTime( ) << " [Cp] Checkpoint loading is in progress..." << std::endl;

   std::ifstream checkpointFile( checkpointFilePath.data( ) );
   if ( !checkpointFile.is_open( ) )
      throw IOException( "The checkpoint file cannot be opened." );
   int numCrawledTotal, frontierSize;
   checkpointFile >> numCrawledTotal >> frontierSize;
   _numCrawledTotal = numCrawledTotal;
   _frontier.reserve( frontierSize );
   for ( auto i = 0; i < frontierSize; ++i )
      {
      String urlString;
      checkpointFile >> urlString;
          try {
              _frontier.emplace( urlString );
          } catch (...) {
              continue;
          }
      }
   checkpointFile >> std::ws >> _scheduledUrls;

   const auto now = std::chrono::steady_clock::now( );
   const auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>( now - beginTime ).count( );
   std::cout << putCurrentDateTime( ) << " [Cp] Checkpoint loading has been finished in " << elapsedTime << " s ["
             << fileSizeToString( std::filesystem::file_size( checkpointFilePath ) ) << "]." << std::endl;
   }

void Crawler::beginCrawl( int numThreads )
   {
   if ( _isRunning )
      throw InvalidOperationException( "The crawler is already running." );
   _isRunning = true;

   for ( auto i = 0; i < numThreads; ++i )
      {
      Thread workerThread( &Crawler::doWork, this, i, numThreads );
//      workerThread.setName( STRING( "Crawler-" << i ) );
      _threadPool.emplace_back( std::move( workerThread ) );
      }

   _gcThread = Thread(
         [ & ]( )
            {
            while ( _isRunning )
               {
               std::this_thread::sleep_for( std::chrono::seconds(_garbageCollectionInterval) );

               UniqueLock frontierLock( _frontierMutex );
               if ( _frontier.size( ) > _frontierSizeLimit )
                  while ( _frontier.size( ) > _frontierSizeLimit / 2 )
                     _frontier.erase( _frontier.cbegin( ) );
               frontierLock.unlock( );

               UniqueLock hitsCacheLock( _hitsCacheMutex );
               _hitsCache.clear( );
               }
            } );
//   _gcThread.setName( "Crawler-GC" );

   _statsThread = Thread(
         [ & ]( )
            {
            while ( _isRunning )
               {
               const auto beginTime = std::chrono::steady_clock::now( );
               std::this_thread::sleep_for( std::chrono::seconds(_config.statsRefreshInterval) );

               UniqueLock frontierLock( _frontierMutex );
               const auto now = std::chrono::steady_clock::now( );
               const auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>( now - beginTime ).count( );
               const auto speed = _numCrawledDuringLastInterval / elapsedTime;
               std::cout << putCurrentDateTime( ) << " [Stats] Speed: " << speed << "/s\t"
                         << "Total: " << _numCrawledTotal << "\tFrontier size: " << _frontier.size( ) << std::endl;
               _numCrawledDuringLastInterval = 0;
               }
            } );
//   _statsThread.setName( "Crawler-Stats" );

   _checkpointThread = Thread(
         [ & ]( )
            {
            while ( _isRunning )
               {
               std::this_thread::sleep_for( std::chrono::seconds(_config.checkpointInterval) );
               createCheckpoint( );
               }
            }
   );
//   _checkpointThread.setName( "Crawler-Cp" );
   }

void Crawler::endCrawl( )
   {
   if ( !_isRunning )
      throw InvalidOperationException( "The crawler is not running." );
   _isRunning = false;

   for ( auto &thread : _threadPool )
      thread.join( );
   _threadPool.clear( );

   _gcThread.join( );
   _statsThread.join( );
   _checkpointThread.join( );
   }

Crawler::Crawler( CrawlerConfiguration config ) :
      _config( std::move( config ) ),
      _logger( StreamWriter::synchronized(
            _config.logPath.has_value( ) ? StreamWriter( _config.logPath.value( ).string( ), true ) :
            StreamWriter( std::clog ) ) ),
      _scheduledUrls( _config.expectedNumUrls, _filterFalsePositiveRate )
   {
   _httpClient.defaultRequestHeaders.accept = "text/html";
   _httpClient.defaultRequestHeaders.acceptEncoding = "identity";
   _httpClient.defaultRequestHeaders.acceptLanguage = "en";
   _httpClient.timeout = 5;

   _htmlParser.linkFilter = [ & ]( const Url &url, const TagInfo &tagInfo )
      { return Crawler::filterLink( url, tagInfo ); };
   }

void Crawler::doWork( int threadId, int numThreads )
   {
   const auto log = [ & ]( StringView value )
      {
      const int width = std::floor( std::log10( numThreads ) + 1 );
      _logger->writeLine(
            STRING( "[Thread-" << std::setw( width ) << std::setfill( '0' ) << threadId << "] " << value ) );
      };

   while ( _isRunning )
      {
      auto urlBatch = getNextUrlBatch( 5 );
      for ( auto &requestUrl : urlBatch )
         {
         if ( !_isRunning ) return;

         HttpResponseMessage response;
         try
            { response = getHttpResponse( requestUrl ); }
         catch ( const HttpRequestException &e )
            {
            const auto message = e.message( );
            if ( message.find( "robots.txt" ) != String::npos )
               log( STRING( "Ign: Disallowed by robots.txt " << requestUrl ) );
            else log( STRING( "Err: HttpRequestException (" << message << ") " << requestUrl ) );
            continue;
            }
         catch ( const NotImplementedException &e )
            {
            log( STRING( "Err: NotImplementedException (" << e.message( ) << ") " << requestUrl ) );
            continue;
            }

         // Ignores non-English contents.
         if ( const auto &contentLanguage = response.headers.contentLanguage;
               contentLanguage.has_value( ) && contentLanguage->find( "en" ) == String::npos )
            {
            log( STRING( "Ign: Content language not English "
                               << requestUrl << " [" << fileSizeToString( response.content.size( ) ) << "]" ) );
            continue;
            }

         // Ignores non-HTML contents.
         if ( const auto &contentType = response.headers.contentType;
               contentType.has_value( ) && contentType->find( "text/html" ) == String::npos )
            {
            log( STRING( "Ign: Content type not HTML "
                               << requestUrl << " [" << fileSizeToString( response.content.size( ) ) << "]" ) );
            continue;
            }


         HtmlInfo htmlInfo;
         try
            { htmlInfo = _htmlParser.parse( response.content ); }
         catch ( const FormatException &e )
            {
            log( STRING( "Err: FormatException ("
                               << e.message( ) << ") " << requestUrl << " ["
                               << fileSizeToString( response.content.size( ) ) << "]" ) );
            continue;
            }

         static const int numWidth = std::floor( std::log10( std::numeric_limits<int>::max( ) ) + 1 );
         const auto htmlInfoFileName = STRING(
               std::setw( numWidth ) << std::setfill( '0' ) << _numCrawledTotal.fetch_add( 1 ) << ".txt" );
         std::ofstream htmlInfoFile( _config.dataDir / htmlInfoFileName );
         if ( !htmlInfoFile.is_open( ) ) throw IOException( "The HTML info file cannot be opened." );
         htmlInfoFile << requestUrl << '\n' << htmlInfo;

         ++_numCrawledDuringLastInterval;
         log( STRING( "Get: " << requestUrl << " [" << fileSizeToString( response.content.size( ) ) << "]" ) );

//         UniqueLock frontierLock( _frontierMutex ), _scheduledUrlsLock( _scheduledUrlsMutex );
         UniqueLock _scheduledUrlsLock( _scheduledUrlsMutex );
         for ( const auto &linkInfo : htmlInfo.links )
            {
             Url url;
           try {
             url = linkInfo.url.isAbsoluteUrl( ) ? linkInfo.url : Url( requestUrl, linkInfo.url );
           } catch (...) {
             continue;
           }
           if ( !_scheduledUrls.contains( url ) )
               {
                 _scheduledUrlsLock.unlock();
                 _distributed->sendURL(url);
                 _scheduledUrlsLock.lock();
//                 _cv.notifyOne( );
               }
            }
         }
      }
   }

int Crawler::getUrlScore( const Url &url )
   {
   auto score = 0;

   // Prefers https scheme.
   if ( url.scheme( ) == "https" ) score++;

   // Prefers shorter host.
   if ( url.host( ).size( ) <= 20 ) score++;

   // Prefers certain domains.
   static const Vector<StringView> preferredDomains = { ".edu", ".gov", ".org" };
   for ( auto preferredDomain : preferredDomains )
      if ( url.host( ).ends_with( preferredDomain ) )
         {
         score++;
         break;
         }

   // Prefers shorter local path.
   if ( url.localPath( ).size( ) <= 10 ) score++;

   // Prefers fewer non-alphabetic characters in the local path.
   if ( std::count_if( url.localPath( ).cbegin( ), url.localPath( ).cend( ), [ ]( char c )
      { return !std::isalpha( c ); } ) > 10 )
      score--;

   // Prefers no query.
   if ( url.query( ).empty( ) ) score++;
   if ( url.query( ).size( ) > 20 ) score--;
   if ( url.query( ).size( ) > 40 ) score--;

   return score;
   }

Vector<Url> Crawler::getNextUrlBatch( int batchSize, int sampleFactor )
   {
   const auto sampleSize = batchSize * sampleFactor;
   Vector<Url> urlBatch;
   urlBatch.reserve( sampleSize );

   UniqueLock frontierLock( _frontierMutex );
   _cv.wait( frontierLock, [ & ]( )
      { return _frontier.size( ) >= sampleSize; } );

   UniqueLock scheduledUrlsLock( _scheduledUrlsMutex ), hitsCacheLock( _hitsCacheMutex );
   for ( auto it = _frontier.cbegin( ); it != _frontier.cend( ) && urlBatch.size( ) < sampleSize; )
      {
      const auto &url = *it;
      if ( _scheduledUrls.contains( url ) )
         {
         it = _frontier.erase( it );
         continue;
         }

      if ( auto &numHits = _hitsCache[ url.host( ) ]; numHits < _hostHitRateLimit )
         {
         ++numHits;
         urlBatch.emplace_back( url );
         it = _frontier.erase( it );
         }
      else ++it;
      }

   hitsCacheLock.unlock( ), scheduledUrlsLock.unlock( ), frontierLock.unlock( );
   std::sort( urlBatch.begin( ), urlBatch.end( ), [ ]( const Url &lhs, const Url &rhs )
      { return getUrlScore( lhs ) > getUrlScore( rhs ); } );

   frontierLock.lock( );
   while ( urlBatch.size( ) > batchSize )
      {
      _frontier.emplace( std::move( urlBatch.back( ) ) );
      urlBatch.pop_back( );
      }
   frontierLock.unlock( );

   scheduledUrlsLock.lock( );
   for ( const auto &url : urlBatch )
      _scheduledUrls.insert( url );

   return urlBatch;
   }

HttpResponseMessage Crawler::getHttpResponse( Url &requestUrl )
   {
//   static constexpr auto maxNumRedirects = 5;
//   for ( auto i = 0; i < maxNumRedirects; ++i )
//      {
   // Conforms to robots.txt.
   if ( !_robotsCatalog.isAllowed( requestUrl ) )
      throw HttpRequestException( "The request URL is disallowed by robots.txt." );

   auto response = _httpClient.get( requestUrl );

   // Handles 301 Moved Permanently and 308 Permanent Redirect.
   if ( response.statusCode == 301 || response.statusCode == 308 )
      {
      if ( !response.headers.location.has_value( ) )
         throw HttpRequestException( "The HTTP response message is malformed." );
      requestUrl = [ & ]( )
         {
         try
            {
            auto redirectedUrl = Url( response.headers.location.value( ) );
            if ( !redirectedUrl.isAbsoluteUrl( ) ) redirectedUrl = Url( requestUrl, redirectedUrl );
            return redirectedUrl;
            }
         catch ( const FormatException &e )
            { throw HttpRequestException( "The redirected URL is malformed.", e ); }
         catch ( ... )
            { throw HttpRequestException( "The redirected URL is malformed." ); }
         }( );

//      UniqueLock lock( _scheduledUrlsMutex );
//      _scheduledUrls.insert( requestUrl );
//      lock.unlock();
      _distributed->sendURL(requestUrl);
      throw HttpRequestException( "Encountering redirected page" );
      }

   if ( response.statusCode != 200 )
      throw HttpRequestException( STRING( "Failed with status code " << response.statusCode << '.' ) );

   return response;
//      }
//   throw HttpRequestException( "Too many redirects." );
   }

bool Crawler::filterLink( const Url &url, const TagInfo &tagInfo )
   {
   // Filers out non-HTML contents by URL suffix.
   static const HashSet<StringView> nonHtmlExtensions = {
         "gif",
         "jpeg",
         "jpg",
         "json",
         "mp3",
         "mp4",
         "ogg",
         "ogv",
         "pdf",
         "png",
         "rdf",
         "rss",
         "svg",
         "tiff",
         "ttf",
         "txt",
         "webm",
         "xml",
         "zip",
   };
   const auto &localPath = url.localPath( );
   if ( const auto pos = localPath.rfind( '.' ); pos != String::npos )
      {
      auto suffix = localPath.substr( pos + 1 );
      for ( auto &c : suffix ) c = toLower( c );
      if ( nonHtmlExtensions.contains( suffix ) ) return false;
      }

   // Filters out non-English contents by tag information.
   auto language = tagInfo.valueOf( "hreflang" );
   if ( !language.has_value( ) ) language = tagInfo.valueOf( "lang" );
   if ( language.has_value( ) && language->find( "en" ) == String::npos ) return false;

   // Filters out non-English contents by URL prefix.
   static const HashSet<StringView> nonEnglishLanguages = {
         "aa",
         "ab",
         "ace",
         "af",
         "ak",
         "als",
         "am",
         "an",
         "ang",
         "ar",
         "arc",
         "arz",
         "as",
         "ast",
         "az",
         "azb",
         "ba",
         "bar",
         "bcl",
         "be",
         "be-tarask",
         "bg",
         "bh",
         "bn",
         "br",
         "bs",
         "ca",
         "ce",
         "ceb",
         "chr",
         "cs",
         "csb",
         "cy",
         "da",
         "de",
         "diq",
         "el",
         "eo",
         "es",
         "et",
         "eu",
         "fa",
         "fi",
         "fo",
         "fr",
         "frr",
         "fy",
         "ga",
         "gd",
         "gl",
         "gn",
         "gom",
         "gu",
         "ha",
         "hak",
         "he",
         "hi",
         "hr",
         "hsb",
         "ht",
         "hu",
         "hy",
         "hyw",
         "ia",
         "id",
         "ie",
         "io",
         "is",
         "it",
         "ja",
         "jv",
         "ka",
         "kk",
         "kl",
         "kn",
         "ko",
         "ks",
         "ku",
         "ky",
         "la",
         "lad",
         "li",
         "lij",
         "lo",
         "lt",
         "lv",
         "mg",
         "min",
         "mk",
         "ml",
         "mr",
         "ms",
         "mt",
         "my",
         "na",
         "nah",
         "nap",
         "nl",
         "nn",
         "no",
         "oc",
         "or",
         "pa",
         "pfl",
         "pl",
         "pms",
         "ps",
         "pt",
         "ro",
         "ru",
         "sa",
         "sah",
         "sd",
         "sh",
         "sk",
         "sl",
         "sq",
         "sr",
         "sv",
         "sw",
         "ta",
         "te",
         "tg",
         "th",
         "tr",
         "tt",
         "uk",
         "ur",
         "uz",
         "vec",
         "vi",
         "vo",
         "wa",
         "war",
         "yi",
         "zh",
         "zh-min-nan",
         "zh-yue",
   };
   auto prefix = url.host( ).substr( 0, url.host( ).find( '.' ) );
   for ( auto &c : prefix ) c = toLower( c );
   if ( nonEnglishLanguages.contains( prefix ) ) return false;

   return true;
   }

void Crawler::createCheckpoint( ) const
   {
   UniqueLock frontierLock( _frontierMutex ), scheduledUrlsLock( _scheduledUrlsMutex );

   const auto beginTime = std::chrono::steady_clock::now( );
   std::cout << putCurrentDateTime( ) << " [Cp] Checkpoint creation is in progress..." << std::endl;

   const auto tempFilePath = std::filesystem::temp_directory_path( ) / _config.checkpointPath.filename( );
   std::ofstream tempFile( tempFilePath );
   if ( !tempFile.is_open( ) ) throw IOException( "The temporary checkpoint file cannot be opened." );
   tempFile << _numCrawledTotal << ' ' << _frontier.size( ) << '\n';
   for ( const auto &url : _frontier ) tempFile << url << '\n';
   tempFile << _scheduledUrls << std::endl;
   tempFile.close( );

   std::filesystem::copy_file( tempFilePath, _config.checkpointPath,
                               std::filesystem::copy_options::overwrite_existing );
   std::filesystem::remove( tempFilePath );

   const auto now = std::chrono::steady_clock::now( );
   const auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>( now - beginTime ).count( );
   std::cout << putCurrentDateTime( ) << " [Cp] Checkpoint creation has been finished in " << elapsedTime << " s ["
             << fileSizeToString( std::filesystem::file_size( _config.checkpointPath ) ) << "]." << std::endl;
   }

void Crawler::insertFrontier(const Url &url) {
  UniqueLock frontierLock( _frontierMutex ), scheduleLock (_scheduledUrlsMutex);
  if ( !_scheduledUrls.contains( url ) )
  {
    _frontier.emplace( url );
    _cv.notifyOne( );
  }
}

void Crawler::setDistributed( Distributed* distributed )
{
  _distributed = distributed;
}