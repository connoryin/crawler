#include <algorithm>

#include "crawler/robots_catalog.h"

RobotsCatalog::RobotsCatalog( )
   {
   _httpClient.defaultRequestHeaders.accept = "text/plain";
   _httpClient.defaultRequestHeaders.acceptEncoding = "identity";
   _httpClient.timeout = 5;

   _cacheThread = Thread(
         [ & ]( )
            {
            while ( _isRunning )
               {
               std::this_thread::sleep_for( std::chrono::seconds(_cacheRefreshInterval) );
//                 Thread::CurrentThread::sleep( _cacheRefreshInterval );
               refreshCache( );
               }
            } );
//   _cacheThread.setName( "Robots-Cache" );
   }

bool RobotsCatalog::isAllowed( const Url &requestUrl )
   {
   UniqueLock lock( _rulesCacheMutex );

   // Fetches and parses robots.txt if it is unknown.
   if ( !_rulesCache.contains( requestUrl.host( ) ) )
      {
      lock.unlock( );

      const auto robotsString = [ & ]( ) -> String
         {
         try
            { return _httpClient.getString( Url( requestUrl, "/robots.txt" ) ); }
         catch ( const HttpRequestException & )
            { return ""; }
         }( );
      auto rules = parseRobotsFile( robotsString );

      lock.lock( );
      _rulesCache.emplace( requestUrl.host( ), std::move( rules ) );
      }

   ++_rulesCache.at( requestUrl.host( ) ).numHits;
   const auto rules = _rulesCache.at( requestUrl.host( ) ).rules;
   lock.unlock( );

   bool isDisallowed = false;
   for ( const auto &rule : rules )
      if ( startsWithPattern( requestUrl.localPath( ), rule.pattern ) )
         {
         if ( rule.type == RuleType::Allow ) return true;
         else if ( rule.type == RuleType::Disallow ) isDisallowed = true;
         else __builtin_unreachable( );
         }
   return !isDisallowed;
   }

Vector<RobotsCatalog::Rule> RobotsCatalog::parseRobotsFile( StringView robotsString )
   {
   Vector<Rule> rules;
   String currentUserAgent;

   std::istringstream robotsStream( String{ robotsString } );
   for ( String line; std::getline( robotsStream, line ); )
      {
      if ( line.empty( ) ) continue;
      if ( std::isspace( line.back( ) ) ) line.pop_back( );
      if ( line.starts_with( '#' ) ) continue;

      auto pos = line.find( ':' );
      if ( pos == String::npos ) continue;

      auto name = line.substr( 0, pos );
      for ( auto &c : name ) c = toLower( c );

      if ( ( pos += 2 ) >= line.size( ) ) continue;
      auto value = line.substr( pos );

      if ( name == "user-agent" ) currentUserAgent = value;
      if ( currentUserAgent != "*" ) continue;

      if ( name == "allow" ) rules.emplace_back( Rule{ RuleType::Allow, std::move( value ) } );
      else if ( name == "disallow" ) rules.emplace_back( Rule{ RuleType::Disallow, std::move( value ) } );
      }
   return rules;
   }

bool RobotsCatalog::startsWithPattern( StringView string, StringView pattern )
   {
   if ( pattern.empty( ) ) return true;
   if ( string.empty( ) ) return pattern == "*";

   if ( string.at( 0 ) == pattern.at( 0 ) )
      return startsWithPattern( string.substr( 1 ), pattern.substr( 1 ) );
   if ( pattern.at( 0 ) == '*' )
      return startsWithPattern( string, pattern.substr( 1 ) ) || startsWithPattern( string.substr( 1 ), pattern );
   return false;
   }

void RobotsCatalog::refreshCache( )
   {
   UniqueLock lock( _rulesCacheMutex );
   for ( auto &&[_, cacheEntry] : _rulesCache )
      cacheEntry.numHits = std::max( cacheEntry.numHits - _cacheHitRateThreshold * _cacheRefreshInterval, 0 );
   std::erase_if( _rulesCache, [ & ]( const std::pair<String, CacheEntry> &pair )
      { return pair.second.numHits == 0; } );
   }
