#pragma once

#include <iostream>
#include <optional>

#include "core/exception.h"
#include "core/net/socket.h"
#include "core/net/ssl.h"
#include "core/net/url.h"
#include "core/string.h"

/// Represents the collection of HTTP request headers.
struct HttpRequestHeaders
   {
   public:
      std::optional<String> accept; ///< The `Accept` header.
      std::optional<String> acceptEncoding; ///< The `Accept-Encoding` header.
      std::optional<String> acceptLanguage; ///< The `Accept-Language` header.
      std::optional<String> connection; ///< The `Connection` header.
      String host; ///< The `Host` header.
      std::optional<String> userAgent; ///< The `User-Agent` header.

      friend std::ostream &operator<<( std::ostream &stream, const HttpRequestHeaders &headers );
   };

/// Represents an HTTP request message.
struct HttpRequestMessage
   {
   public:
      String method; ///< The HTTP method.
      String version = "1.1"; ///< The HTTP message version.
      HttpRequestHeaders headers; ///< The collection of HTTP request headers.
      String content; ///< The content of the HTTP message.

      /// Initializes an `HttpRequestMessage` with the specified HTTP method and request URL.
      /// \param method The HTTP method.
      /// \param requestUrl The request URL.
      /// \throw ArgumentException The request URL is invalid.
      HttpRequestMessage( String method, Url requestUrl ) :
            method( std::move( method ) ), _requestUrl( std::move( requestUrl ) )
         { setRequestUrl( std::move( _requestUrl ) ); }

      /// Initializes an `HttpRequestMessage` with the specified HTTP method and request URL string.
      /// \param method The HTTP method.
      /// \param requestUrl The request URL string.
      /// \throw ArgumentException The request URL string is invalid.
      HttpRequestMessage( String method, StringView requestUrl ) :
            method( std::move( method ) ), _requestUrl( requestUrl )
         { setRequestUrl( std::move( _requestUrl ) ); }

      /// Gets the request URL.
      /// \return THe request URL.
      [[nodiscard]] const Url &requestUrl( ) const noexcept
         { return _requestUrl; }

      /// Sets the request URL.
      /// \param value The request URL.
      /// \throw ArgumentException The request URL is invalid.
      void setRequestUrl( Url value )
         {
         _requestUrl = std::move( value );
         if ( !_requestUrl.isAbsoluteUrl( ) )
            throw ArgumentException( "The request URL is not an absolute URL" );
         if ( _requestUrl.scheme( ) != "http" && _requestUrl.scheme( ) != "https" )
            throw ArgumentException( "The request URL scheme is invalid." );
         headers.host = _requestUrl.host( );
         }

      /// Sets the request URL.
      /// \param value The request URL string.
      /// \throw ArgumentException The request URL string is invalid.
      void setRequestUrl( StringView value )
         { setRequestUrl( Url( value ) ); }

      friend std::ostream &operator<<( std::ostream &stream, const HttpRequestMessage &request );

   private:
      Url _requestUrl;
   };

/// Represents the collection of HTTP response headers.
struct HttpResponseHeaders
   {
   public:
      std::optional<String> contentLanguage; ///< The `Content-Language` header.
      std::optional<String> contentType; ///< The `Content-Type` header.
      std::optional<String> location; ///< The `Location` header.

      /// Appends the specified value to an HTTP response header.
      /// \param header The HTTP response header.
      /// \param value The value to append.
      static void appendValue( std::optional<String> &header, StringView value )
         {
         if ( !header.has_value( ) ) header = value;
         else header->append( ", " ), header->append( value );
         }

      friend std::istream &operator>>( std::istream &stream, HttpResponseHeaders &headers );

      friend std::ostream &operator<<( std::ostream &stream, const HttpResponseHeaders &headers );
   };

/// Represents an HTTP response message.
struct HttpResponseMessage
   {
   public:
      String version; ///< The HTTP message version.
      int statusCode; ///< The status code.
      String reasonPhrase; ///< The reason phrase.
      HttpResponseHeaders headers; ///< The collection of HTTP response headers.
      String content; ///< The content of the HTTP message.

      friend std::istream &operator>>( std::istream &stream, HttpResponseMessage &response );

      friend std::ostream &operator<<( std::ostream &stream, const HttpResponseMessage &response );
   };

/// The exception that is thrown when an HTTP request error occurs.
struct HttpRequestException : public Exception
   {
   public:
      using Exception::Exception;
   };

/// Provides a class for sending HTTP requests and receiving HTTP responses.
class HttpClient
   {
   public:
      /// The headers sent with each request.
      HttpRequestHeaders defaultRequestHeaders{
            .connection = "close",
            .userAgent = "UMichBot"
      };
      int timeout = 60; ///< The time to wait in seconds before the request times out.

      /// Sends an HTTP request.
      /// \param request The HTTP request message.
      /// \return The HTTP response message.
      /// \throw HttpRequestException The HTTP request failed.
      [[nodiscard]] HttpResponseMessage send( HttpRequestMessage request ) const;

      /// Sends a GET request to the specified URL.
      /// \param requestUrl The request URL.
      /// \return The HTTP response message.
      /// \throw HttpRequestException The HTTP request failed.
      [[nodiscard]] HttpResponseMessage get( const Url &requestUrl ) const
         { return send( HttpRequestMessage( "GET", requestUrl ) ); }

      /// Sends a GET request to the specified URL.
      /// \param requestUrl The request URL string.
      /// \return The HTTP response message.
      /// \throw HttpRequestException The HTTP request failed.
      [[nodiscard]] HttpResponseMessage get( StringView requestUrl ) const
         { return get( Url( requestUrl ) ); }

      /// Sends a GET request to the specified URL and returns the response content.
      /// \param requestUrl The request URL.
      /// \return The HTTP response content.
      /// \throw HttpRequestException The HTTP request failed.
      [[nodiscard]] String getString( const Url &requestUrl ) const
         { return get( requestUrl ).content; }

      /// Sends a GET request to the specified URL and returns the response content.
      /// \param requestUrl The request URL string.
      /// \return The HTTP response content.
      /// \throw HttpRequestException The HTTP request failed.
      [[nodiscard]] String getString( StringView requestUrl ) const
         { return getString( Url( requestUrl ) ); }
   };
