#pragma once

#include <optional>

#include "core/exception.h"
#include "core/hash_table.h"
#include "core/io.h"
#include "core/string.h"

/// Represents a Uniform Resource Locator (URL) and provides easy access to parts of the URL.
struct Url
   {
   public:
      /// Initializes a `Url` with the specified URL string.
      /// \param urlString A URL string.
      /// \throw FormatException The URL string is malformed.
      /// \throw NotImplementedException The URL scheme is not supported.
      explicit Url( StringView urlString = "" );

      /// Initializes a `Url` based on the combination of the specified base URL and relative URL.
      /// \param baseUrl The base URL.
      /// \param relativeUrl The relative URL.
      /// \throw ArgumentException The base URL is not an absolute URL.
      Url( const Url &baseUrl, const Url &relativeUrl ) : Url( baseUrl, relativeUrl._urlString )
         { }

      /// Initializes a `Url` based on the combination of the specified base URL and relative URL string.
      /// \param baseUrl The base URL.
      /// \param relativeUrl The relative URL string.
      /// \throw ArgumentException The base URL is not an absolute URL.
      Url( const Url &baseUrl, StringView relativeUrl );

      /// Indicates if the `Url` is absolute.
      /// \return `true` if the `Url` contains a scheme, an authority, and a local path.
      [[nodiscard]] bool isAbsoluteUrl( ) const noexcept
         { return _isAbsoluteUrl; }

      /// Gets the scheme of the `Url`.
      /// \return The scheme of the `Url`, converted to lowercase.
      /// \throw InvalidOperationException The `Url` is not an absolute URL.
      [[nodiscard]] const String &scheme( ) const
         {
         if ( !_isAbsoluteUrl ) throw InvalidOperationException( "The URL is not an absolute URL" );
         return _scheme.value( );
         }

      /// Gets the host of the `Url`.
      /// \return The host of the `Url`.
      /// \throw InvalidOperationException The `Url` is not an absolute URL.
      [[nodiscard]] const String &host( ) const
         {
         if ( !_isAbsoluteUrl ) throw InvalidOperationException( "The URL is not an absolute URL" );
         return _host.value( );
         }

      /// Gets the port of the `Url`.
      /// \return The port of the `Url` if specified, otherwise the default value for the scheme.
      /// \throw InvalidOperationException The `Url` is not an absolute URL.
      [[nodiscard]] int port( ) const
         {
         if ( !_isAbsoluteUrl ) throw InvalidOperationException( "The URL is not an absolute URL" );
         return _port.value( );
         }

      /// Gets the local path of the `Url`.
      /// \return The local path of the `Url`.
      /// \throw InvalidOperationException The `Url` is not an absolute URL.
      [[nodiscard]] const String &localPath( ) const
         {
         if ( !_isAbsoluteUrl ) throw InvalidOperationException( "The URL is not an absolute URL" );
         return _localPath.value( );
         }

      /// Gets the query of the `Url`.
      /// \return The query of the `Url`.
      /// \throw InvalidOperationException The `Url` is not an absolute URL.
      [[nodiscard]] const String &query( ) const
         {
         if ( !_isAbsoluteUrl ) throw InvalidOperationException( "The URL is not an absolute URL" );
         return _query.value( );
         }

      /// Gets the path and query separated by a question mark.
      /// \return The path and query separated by a question mark.
      /// \throw InvalidOperationException The `Url` is not an absolute URL.
      [[nodiscard]] String pathAndQuery( ) const
         {
         if ( !_isAbsoluteUrl ) throw InvalidOperationException( "The URL is not an absolute URL" );
         return !_query.value( ).empty( ) ? STRING( _localPath.value( ) << '?' << _query.value( ) )
                                          : _localPath.value( );
         }

      bool operator==( const Url &rhs ) const noexcept
         { return _urlString == rhs._urlString; }
      bool operator!=( const Url &rhs ) const noexcept
         { return _urlString != rhs._urlString; }

      friend std::istream &operator>>( std::istream &stream, Url &url )
         {
         String urlString;
         stream >> urlString;
         url = Url( urlString );
         return stream;
         }

      friend std::ostream &operator<<( std::ostream &stream, const Url &url )
         { return stream << url._urlString; }

      friend Hash<Url>;

   private:
      void canonicalize( );

      inline static const HashMap<StringView, int> _defaultPorts{
            { "http",  80 },
            { "https", 443 }
      };

      String _urlString;
      bool _isAbsoluteUrl;
      std::optional<String> _scheme;
      std::optional<String> _host;
      std::optional<int> _port;
      std::optional<String> _localPath;
      std::optional<String> _query;
   };

template<>
struct Hash<Url>
   {
   public:
      size_t operator()( const Url &url ) const
         { return Hash<String>( )( url._urlString ); }
   };
