#include "core/net/ssl.h"

SslStream::SslStream( const Socket &socket )
   {
   _context.reset( SSL_CTX_new( TLS_method( ) ) );
   if ( _context == nullptr )
      throw SslException( );

   if ( SSL_CTX_set_default_verify_paths( _context.get( ) ) == 0 )
      throw SslException( );

   _ssl.reset( SSL_new( _context.get( ) ) );
   if ( _ssl == nullptr )
      throw SslException( );

   if ( SSL_set_fd( _ssl.get( ), socket.handle( ) ) == 0 )
      throw SslException( );
   }

SslStream &SslStream::operator=( SslStream &&other ) noexcept
   {
   if ( this != &other )
      {
      if ( _ssl != nullptr ) SSL_shutdown( _ssl.get( ) );
      _context = std::move( other._context );
      _ssl = std::move( other._ssl );
      }
   return *this;
   }

void SslStream::authenticateAsClient( )
   {
   const auto returnCode = SSL_connect( _ssl.get( ) );
   if ( returnCode != 1 )
      throw SslException( SSL_get_error( _ssl.get( ), returnCode ) );
   }

int SslStream::write( const std::byte *buffer, int count ) const
   {
   size_t numBytesWritten = 0;
   const auto returnCode = SSL_write_ex( _ssl.get( ), buffer, count, &numBytesWritten );
   if ( returnCode == 0 )
      throw SslException( SSL_get_error( _ssl.get( ), returnCode ) );
   return numBytesWritten;
   }

int SslStream::read( std::byte *buffer, int count )
   {
   size_t numBytesRead = 0;
   const auto returnCode = SSL_read_ex( _ssl.get( ), buffer, count, &numBytesRead );
   if ( returnCode == 0 )
      {
      const auto errorCode = SSL_get_error( _ssl.get( ), returnCode );
      if ( !( errorCode == SSL_ERROR_SYSCALL && errno == 0 ) )
         throw SslException( errorCode );
      }
   return numBytesRead;
   }

void SslStream::shutdown( )
   {
   const auto returnCode = SSL_shutdown( _ssl.get( ) );
   if ( returnCode < 0 )
      throw SslException( SSL_get_error( _ssl.get( ), returnCode ) );
   _ssl.reset( nullptr );
   }
