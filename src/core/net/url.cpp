#include "core/net/url.h"

Url::Url( StringView urlString )
   {
   auto beginPos = 0;
   auto endPos = urlString.find( "//" );

   if ( endPos == StringView::npos ) // The URL string is a relative URL.
      {
      _urlString = urlString;
      _isAbsoluteUrl = false;
      return;
      }

   // Parses the scheme.
   if ( endPos-- > 0 ) _scheme = urlString.substr( beginPos, endPos - beginPos );
   else _scheme = "http";
   for ( auto &c : _scheme.value( ) ) c = toLower( c );
   if ( !_defaultPorts.contains( _scheme.value( ) ) )
      throw NotImplementedException( "Only HTTP and HTTPS URLs are supported." );

   // Parses the host.
   if ( ( beginPos = endPos + 3 ) >= urlString.size( ) )
      throw FormatException( "The URL string is malformed." );
   endPos = urlString.find_first_of( ":/", beginPos );
   _host = urlString.substr( beginPos, endPos - beginPos );

   // Parses the port.
   if ( endPos != StringView::npos && urlString.at( endPos ) == ':' )
      {
      beginPos = endPos + 1;
      endPos = urlString.find( '/', beginPos );
      try
         { _port = std::stoi( String( urlString.substr( beginPos, endPos - beginPos ) ) ); }
      catch ( ... )
         { throw FormatException( "The URL string is malformed." ); }
      }
   else _port = _defaultPorts.at( _scheme.value( ) );

   // Parses the local path.
   if ( endPos != StringView::npos )
      {
      beginPos = endPos;
      endPos = urlString.find_first_of( "?#", beginPos );
      _localPath = urlString.substr( beginPos, endPos - beginPos );
      }
   else _localPath = "/";

   // Parses the query.
   if ( endPos != StringView::npos && urlString.at( endPos ) == '?' )
      {
      beginPos = endPos + 1;
      endPos = urlString.find( '#', beginPos );
      _query = urlString.substr( beginPos, endPos - beginPos );
      }
   else _query = "";

   canonicalize( );
   _isAbsoluteUrl = true;
   }

Url::Url( const Url &baseUrl, StringView relativeUrl )
   {
   if ( !baseUrl._isAbsoluteUrl )
      throw ArgumentException( "The base URL is not an absolute URL." );

   _scheme = baseUrl._scheme;
   _host = baseUrl._host;
   _port = baseUrl._port;

   // Parses the local path.
   auto beginPos = 0;
   auto endPos = relativeUrl.find_first_of( "?#" );
   _localPath = relativeUrl.substr( beginPos, endPos - beginPos );
   if ( !relativeUrl.starts_with( '/' ) )
      _localPath->insert( 0, baseUrl._localPath.value( ) );

   // Parses the query.
   if ( endPos != StringView::npos && relativeUrl.at( endPos ) == '?' )
      {
      beginPos = endPos + 1;
      endPos = relativeUrl.find( '#', beginPos );
      _query = relativeUrl.substr( beginPos, endPos - beginPos );
      }
   else _query = "";

   canonicalize( );
   _isAbsoluteUrl = true;
   }

void Url::canonicalize( )
   {
   std::ostringstream stream;
   stream << _scheme.value( ) << "://" << _host.value( );
   if ( _port.value( ) != _defaultPorts.at( _scheme.value( ) ) )
      stream << ':' << _port.value( );
   stream << _localPath.value( );
   if ( !_query.value( ).empty( ) ) stream << '?' << _query.value( );
   _urlString = stream.str( );
   }
