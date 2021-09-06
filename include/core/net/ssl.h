#pragma once

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "core/exception.h"
#include "core/memory.h"
#include "core/net/socket.h"
#include "core/string.h"

/// The exception that is thrown when a SSL error occurs.
struct SslException : public SystemException
   {
   public:
      /// Initializes an `SslException` with the specified error code.
      /// \param errorCode The error code.
      explicit SslException( int errorCode = static_cast<int>(ERR_get_error( )) ) noexcept: SystemException( errorCode )
         { }

      [[nodiscard]] String message( ) const override
         { return ERR_error_string( errorCode( ), nullptr ); }
   };

/// Provides a stream used for client-server communication based on the Secure Socket Layer (SSL) security protocol.
class SslStream
   {
   public:
      /// Initializes an `SslStream` using the specified socket.
      /// \param socket A socket used for sending and receiving data.
      /// \throw SslException An SSL error occurred.
      explicit SslStream( const Socket &socket );

      SslStream( const SslStream & ) = delete;
      SslStream &operator=( const SslStream & ) = delete;

      SslStream( SslStream && ) noexcept = default;
      SslStream &operator=( SslStream &&other ) noexcept;

      /// Authenticates the client side of a connection.
      /// \throw SslException An SSL error occurred.
      void authenticateAsClient( );

      /// Writes the specified number of bytes to the stream.
      /// \param buffer The buffer that holds bytes to be written.
      /// \param count The number of bytes to write.
      /// \return The actual number of bytes written.
      /// \throw SslException An SSL error occurred.
      int write( const std::byte *buffer, int count ) const;

      /// Reads the specified number of bytes from the stream.
      /// \param buffer The buffer to hold received bytes.
      /// \param count The number of bytes to read.
      /// \return The actual number of bytes read.
      /// \throw SslException An SSL error occurred.
      int read( std::byte *buffer, int count );

      /// Shuts down the `SslStream`.
      /// \throw SslException An SSL error occurred.
      void shutdown( );

   private:
      inline static const auto _initCode = SSL_library_init( );

      UniquePtr<SSL_CTX, decltype( &SSL_CTX_free )> _context{ nullptr, SSL_CTX_free };
      UniquePtr<SSL, decltype( &SSL_free )> _ssl{ nullptr, SSL_free };
   };
