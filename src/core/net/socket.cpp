#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "core/net/socket.h"

std::ostream &operator<<( std::ostream &stream, AddressFamily addressFamily )
   {
   switch ( addressFamily )
      {
      case AddressFamily::InterNetwork:
         return stream << "InterNetwork";
      case AddressFamily::Unknown:
         return stream << "Unknown";
      default:
         __builtin_unreachable( );
      }
   }

std::ostream &operator<<( std::ostream &stream, SocketType socketType )
   {
   switch ( socketType )
      {
      case SocketType::Dgram:
         return stream << "Dgram";
      case SocketType::Stream:
         return stream << "Stream";
      case SocketType::Unknown:
         return stream << "Unknown";
      default:
         __builtin_unreachable( );
      }
   }

std::ostream &operator<<( std::ostream &stream, ProtocolType protocolType )
   {
   switch ( protocolType )
      {
      case ProtocolType::Udp:
         return stream << "Udp";
      case ProtocolType::Tcp:
         return stream << "Tcp";
      case ProtocolType::Unknown:
         return stream << "Unknown";
      default:
         __builtin_unreachable( );
      }
   }

std::ostream &operator<<( std::ostream &stream, SocketOptionLevel optionLevel )
   {
   switch ( optionLevel )
      {
      case SocketOptionLevel::Socket:
         return stream << "Socket";
      default:
         __builtin_unreachable( );
      }
   }

std::ostream &operator<<( std::ostream &stream, SocketOptionName optionName )
   {
   switch ( optionName )
      {
      case SocketOptionName::ReuseAddress:
         return stream << "ReuseAddress";
      default:
         __builtin_unreachable( );
      }
   }

std::ostream &operator<<( std::ostream &stream, SocketFlags socketFlags )
   {
   if ( socketFlags == SocketFlags::None )
      return stream << "None";

   const auto value = static_cast<unsigned>(socketFlags);
   Vector<StringView> names;
   if ( value & static_cast<unsigned>(SocketFlags::Peek) )
      names.emplace_back( "Peek" );
   if ( value & static_cast<unsigned>(SocketFlags::WaitAll) )
      names.emplace_back( "WaitAll" );

   stream << names[ 0 ];
   for ( auto it = std::next( names.cbegin( ) ); it != names.cend( ); ++it )
      stream << ", " << *it;
   return stream;
   }

Vector<IPAddress> Dns::getHostAddresses( StringView hostNameOrAddress )
   {
   const addrinfo requirements{
         .ai_family = static_cast<int>(AddressFamily::InterNetwork),
         .ai_socktype = static_cast<int>(SocketType::Stream),
         .ai_protocol = static_cast<int>(ProtocolType::Tcp)
   };
   addrinfo *addressInfo;
   const auto errorCode = getaddrinfo( hostNameOrAddress.data( ), nullptr, &requirements, &addressInfo );
   if ( errorCode != 0 )
      throw SocketException( );

   Vector<IPAddress> addresses;
   for ( auto it = addressInfo; it != nullptr; it = it->ai_next )
      addresses.emplace_back( reinterpret_cast<const sockaddr_in *>(it->ai_addr)->sin_addr.s_addr );
   freeaddrinfo( addressInfo );

   return addresses;
   }

Socket::Socket( AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType ) :
      Socket( addressFamily, socketType, protocolType,
              socket( static_cast<int>(addressFamily), static_cast<int>(socketType), static_cast<int>(protocolType) ) )
   {
   if ( _handle == -1 )
      throw SocketException( );
   }

Socket::Socket( Socket &&other ) noexcept:
      _addressFamily( other._addressFamily ), _socketType( other._socketType ), _protocolType( other._protocolType ),
      _handle( std::exchange( other._handle, -1 ) ),
      _localEP( std::move( other._localEP ) ), _remoteEP( std::move( other._remoteEP ) )
   { }
Socket &Socket::operator=( Socket &&other ) noexcept
   {
   if ( this != &other )
      {
      if ( _handle != -1 ) close( );
      _addressFamily = other._addressFamily;
      _socketType = other._socketType;
      _protocolType = other._protocolType;
      _handle = std::exchange( other._handle, -1 );
      _localEP = std::move( other._localEP );
      _remoteEP = std::move( other._remoteEP );
      }
   return *this;
   }

int Socket::available( ) const
   {
   int numBytes;
   if ( ioctl( _handle, FIONREAD, &numBytes ) == -1 )
      throw SocketException( );
   return numBytes;
   }

void Socket::bind( const IPEndPoint &localEP )
   {
   const auto socketAddress = localEP.serialize( );
   if ( ::bind( _handle, &socketAddress, sizeof( sockaddr_in ) ) == -1 )
      throw SocketException( );
   _localEP.emplace( localEP );
   }

void Socket::listen( int backlog ) const
   {
   if ( ::listen( _handle, backlog ) == -1 )
      throw SocketException( );
   }

void Socket::connect( StringView host, int port )
   {
   const auto addressList = Dns::getHostAddresses( host );
   std::exception_ptr exceptionPtr;
   for ( auto address : addressList )
      {
      try
         {
         return connect( address, port );
         }
      catch ( ... )
         {
         exceptionPtr = std::current_exception( );
         }
      }
   std::rethrow_exception( exceptionPtr );
   }

void Socket::connect( IPAddress address, int port )
   {
   const IPEndPoint remoteEP( address, port );
   auto socketAddress = remoteEP.serialize( );
   socklen_t addressLength = sizeof( sockaddr_in );

   if ( ::connect( _handle, &socketAddress, addressLength ) == -1 )
      throw SocketException( );
   _remoteEP.emplace( remoteEP );

   if ( getsockname( _handle, &socketAddress, &addressLength ) == -1 )
      throw SocketException( );
   _localEP.emplace( IPEndPoint::create( socketAddress ) );
   }

void Socket::connect( const EndPoint &remoteEP )
   {
   if ( const auto *const dnsRemoteEP = dynamic_cast<const DnsEndPoint *>(&remoteEP); dnsRemoteEP != nullptr )
      connect( dnsRemoteEP->host, dnsRemoteEP->port );
   else if ( const auto *const ipRemoteEP = dynamic_cast<const IPEndPoint *>(&remoteEP); ipRemoteEP != nullptr )
      connect( ipRemoteEP->address, ipRemoteEP->port );
   else __builtin_unreachable( );
   }

Socket Socket::accept( )
   {
   SocketAddress socketAddress{ };
   socklen_t addressLength = sizeof( sockaddr_in );
   const auto handle = ::accept( _handle, &socketAddress, &addressLength );
   if ( handle == -1 )
      throw SocketException( );

   Socket socket( _addressFamily, _socketType, _protocolType, handle );
   if ( _localEP.has_value( ) ) socket._localEP.emplace( *_localEP );
   socket._remoteEP.emplace( IPEndPoint::create( socketAddress ) );
   return socket;
   }

int Socket::send( const std::byte *buffer, int count, SocketFlags socketFlags ) const
   {
   const auto numBytesSent = ::send( _handle, buffer, count, static_cast<int>(socketFlags) );
   if ( numBytesSent == -1 )
      throw SocketException( );
   return numBytesSent;
   }

int Socket::sendTo( const std::byte *buffer, int count, const IPEndPoint &remoteEP, SocketFlags socketFlags )
   {
   auto socketAddress = remoteEP.serialize( );
   socklen_t addressLength = sizeof( sockaddr_in );
   const auto numBytesSent = sendto( _handle, buffer, count, static_cast<int>(socketFlags),
                                     &socketAddress, addressLength );
   if ( numBytesSent == -1 )
      throw SocketException( );

   if ( !_localEP.has_value( ) )
      {
      if ( getsockname( _handle, &socketAddress, &addressLength ) == -1 )
         throw SocketException( );
      _localEP.emplace( IPEndPoint::create( socketAddress ) );
      }

   return numBytesSent;
   }

int Socket::receive( std::byte *buffer, int count, SocketFlags socketFlags ) const
   {
   const auto numBytesReceived = recv( _handle, buffer, count, static_cast<int>(socketFlags) );
   if ( numBytesReceived == -1 )
      throw SocketException( );
   return numBytesReceived;
   }

int Socket::receiveFrom( std::byte *buffer, int count, IPEndPoint *remoteEP, SocketFlags socketFlags )
   {
   SocketAddress socketAddress;
   socklen_t addressLength = sizeof( sockaddr_in );
   const auto numBytesReceived = recvfrom( _handle, buffer, count, static_cast<int>(socketFlags),
                                           &socketAddress, &addressLength );
   if ( numBytesReceived == -1 )
      throw SocketException( );

   if ( remoteEP != nullptr )
      *remoteEP = IPEndPoint::create( socketAddress );

   if ( !_localEP.has_value( ) )
      {
      if ( getsockname( _handle, &socketAddress, &addressLength ) == -1 )
         throw SocketException( );
      _localEP.emplace( IPEndPoint::create( socketAddress ) );
      }

   return numBytesReceived;
   }

void Socket::close( )
   {
   if ( ::close( _handle ) == -1 )
      throw SocketException( );
   _handle = -1;
   }
