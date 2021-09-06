#include <array>

#include "core/net/http.h"
#include "core/time.h"

std::ostream &operator<<( std::ostream &stream, const HttpRequestHeaders &headers )
   {
   if ( headers.accept.has_value( ) )
      stream << "Accept: " << headers.accept.value( ) << "\r\n";
   if ( headers.acceptEncoding.has_value( ) )
      stream << "Accept-Encoding: " << headers.acceptEncoding.value( ) << "\r\n";
   if ( headers.acceptLanguage.has_value( ) )
      stream << "Accept-Language: " << headers.acceptLanguage.value( ) << "\r\n";
   if ( headers.connection.has_value( ) )
      stream << "Connection: " << headers.connection.value( ) << "\r\n";
   stream << "Host: " << headers.host << "\r\n";
   if ( headers.userAgent.has_value( ) )
      stream << "User-Agent: " << headers.userAgent.value( ) << "\r\n";
   return stream;
   }

std::ostream &operator<<( std::ostream &stream, const HttpRequestMessage &request )
   {
   return stream << request.method << " " << request._requestUrl.pathAndQuery( )
                 << " HTTP/" << request.version << "\r\n"
                 << request.headers
                 << "\r\n"
                 << request.content;
   }

std::istream &operator>>( std::istream &stream, HttpResponseHeaders &headers )
   {
   for ( String line; std::getline( stream, line ); )
      {
      if ( line.empty( ) ) throw FormatException( "The HTTP response headers are malformed." );
      if ( line.pop_back( ); line.empty( ) ) break;

      auto pos = line.find( ':' );
      if ( pos == String::npos ) continue;

      auto name = line.substr( 0, pos );
      for ( auto &c : name ) c = toLower( c );

      if ( ( pos += 2 ) >= line.size( ) ) continue;
      auto value = line.substr( pos );

      if ( name == "content-language" ) headers.contentLanguage = std::move( value );
      else if ( name == "content-type" ) HttpResponseHeaders::appendValue( headers.contentType, value );
      else if ( name == "location" ) headers.location = std::move( value );
      }
   return stream;
   }

std::ostream &operator<<( std::ostream &stream, const HttpResponseHeaders &headers )
   {
   if ( headers.contentLanguage.has_value( ) )
      stream << "Content-Language: " << headers.contentLanguage.value( ) << "\r\n";
   if ( headers.contentType.has_value( ) )
      stream << "Content-Type: " << headers.contentLanguage.value( ) << "\r\n";
   if ( headers.location.has_value( ) ) stream << "Location: " << headers.location.value( ) << "\r\n";
   return stream;
   }

std::istream &operator>>( std::istream &stream, HttpResponseMessage &response )
   {
   stream.ignore( 5 );
   stream >> response.version >> response.statusCode;
   std::getline( stream, response.reasonPhrase );
   if ( response.reasonPhrase.empty( ) ) throw FormatException( "The HTTP response message is malformed" );
   response.reasonPhrase.pop_back( );
   stream >> response.headers;
   response.content = String( std::istreambuf_iterator<char>( stream ), std::istreambuf_iterator<char>( ) );
   return stream;
   }

std::ostream &operator<<( std::ostream &stream, const HttpResponseMessage &response )
   {
   return stream << "HTTP/" << response.version << " " << response.statusCode << " " << response.reasonPhrase << "\r\n"
                 << response.headers
                 << "\r\n"
                 << response.content;
   }

HttpResponseMessage HttpClient::send( HttpRequestMessage request ) const
   {
   const auto beginTime = std::chrono::steady_clock::now( );
   const auto checkTimeout = [ & ]( )
      {
      const auto now = std::chrono::steady_clock::now( );
      const auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>( now - beginTime ).count( );
      if ( elapsedTime > timeout )
         throw HttpRequestException( "The request times out." );
      };

   static constexpr auto maxNumRedirects = 5;
   for ( auto i = 0; i < maxNumRedirects; ++i )
      {
      // Updates the HTTP request headers.
      const auto requestUrl = request.requestUrl( );
      request.headers = defaultRequestHeaders;
      request.headers.host = requestUrl.host( );

      // Connects to the host server.
      Socket socket{ AddressFamily::InterNetwork, SocketType::Stream, ProtocolType::Tcp };
      socket.setSendTimeout( timeout );
      socket.setReceiveTimeout( timeout );
      try
         { socket.connect( requestUrl.host( ), requestUrl.port( ) ); }
      catch ( const SocketException &e )
         { throw HttpRequestException( "A network error occurred.", e ); }

      const auto requestString = STRING( request );
      std::stringstream responseStream;

      const auto &scheme = requestUrl.scheme( );
      std::array<std::byte, 4096> buffer{ };
      int numBytesRead;
      if ( scheme == "http" ) // Sends and receives through ordinary TCP.
         {
         try
            {
            socket.send( reinterpret_cast<const std::byte *>(requestString.data( )), requestString.size( ) );
            while ( ( numBytesRead = socket.receive( buffer.data( ), buffer.size( ) ) ) > 0 )
               {
               checkTimeout( );
               responseStream.write( reinterpret_cast<const char *>(buffer.data( )), numBytesRead );
               }
            }
         catch ( const SocketException &e )
            { throw HttpRequestException( "A network error occurred.", e ); }
         }
      else if ( scheme == "https" ) // Sends and receives through SSL.
         {
         try
            {
            SslStream sslStream( socket );
            sslStream.authenticateAsClient( );
            sslStream.write( reinterpret_cast<const std::byte *>(requestString.data( )), requestString.size( ) );
            while ( ( numBytesRead = sslStream.read( buffer.data( ), buffer.size( ) ) ) > 0 )
               {
               checkTimeout( );
               responseStream.write( reinterpret_cast<const char *>(buffer.data( )), numBytesRead );
               }
            }
         catch ( const SslException &e )
            {
            if ( e.errorCode( ) != SSL_ERROR_ZERO_RETURN )
               throw HttpRequestException( "A network error occurred.", e );
            }
         }

      // Parses the HTTP response message.
      HttpResponseMessage response;
      try
         { responseStream >> response; }
      catch ( const FormatException &e )
         { throw HttpRequestException( "The HTTP response message is malformed", e ); }

      // Handles 302 & 307 Temporary Redirect.
      if ( response.statusCode == 302 || response.statusCode == 307 )
         {
         if ( !response.headers.location.has_value( ) )
            throw HttpRequestException( "The HTTP response message is malformed." );
         request.setRequestUrl( [ & ]( )
                                   {
                                   try
                                      {
                                      auto redirectedUrl = Url( response.headers.location.value( ) );
                                      if ( !redirectedUrl.isAbsoluteUrl( ) )
                                         redirectedUrl = Url( requestUrl, redirectedUrl );
                                      return redirectedUrl;
                                      }
                                   catch ( const FormatException &e )
                                      { throw HttpRequestException( "The redirected URL is malformed.", e ); }
                                   }( ) );
         continue;
         }

      if ( response.statusCode != 200 && response.statusCode != 301 && response.statusCode != 308 )
         throw HttpRequestException( STRING( "Failed with status code " << response.statusCode << '.' ) );

      return response;
      }
   throw HttpRequestException( "Too many redirects." );
   }
