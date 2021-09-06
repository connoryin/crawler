#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <optional>
#include <sys/socket.h>
#include <tuple>

#include "core/exception.h"
#include "core/string.h"
#include "core/vector.h"

/// Stores serialized information of socket address.
using SocketAddress = sockaddr;

/// Defines addressing schemes.
enum class AddressFamily
   {
      InterNetwork = AF_INET, ///< Address for IP version 4.
      Unknown = -1 ///< Unknown address family.
   };

std::ostream &operator<<( std::ostream &stream, AddressFamily addressFamily );

/// Defines socket types.
enum class SocketType
   {
      Dgram = SOCK_DGRAM, ///< Supports unreliable, connectionless datagrams.
      Stream = SOCK_STREAM, ///< Supports reliable, connection-based byte streams.
      Unknown = -1 /// Unknown socket type.
   };

std::ostream &operator<<( std::ostream &stream, SocketType socketType );

/// Defines socket protocols.
enum class ProtocolType
   {
      Udp = IPPROTO_UDP, ///< User Datagram Protocol.
      Tcp = IPPROTO_TCP, ///< Transmission Control Protocol.
      Unknown = -1 ///< Unknown protocol.
   };

std::ostream &operator<<( std::ostream &stream, ProtocolType protocolType );

/// Defines socket option levels.
enum class SocketOptionLevel
   {
      Socket = SOL_SOCKET ///< Socket options that apply to all sockets.
   };

std::ostream &operator<<( std::ostream &stream, SocketOptionLevel optionLevel );

/// Defines socket configuration option names.
enum class SocketOptionName
   {
      ReuseAddress = SO_REUSEADDR, ///< Allows the socket to be bound to an address that is already in use.
      SendTimeout = SO_SNDTIMEO, ///< The timeout for sending.
      ReceiveTimeout = SO_RCVTIMEO ///< The timeout for receiving.
   };

std::ostream &operator<<( std::ostream &stream, SocketOptionName optionName );

/// Defines socket sending and receiving behaviors.
enum class SocketFlags
   {
      None = 0, ///< Uses no flags.
      Peek = MSG_PEEK, ///< Peeks at the incoming message.
      WaitAll = MSG_WAITALL, ///< Waits until all bytes are received.
      NoSignal = MSG_NOSIGNAL ///< Does not signal SIGPIPE.
   };

/// The exception that is thrown when a socket error occurs.
struct SocketException : public SystemException
   {
   public:
      using SystemException::SystemException;
   };

/// Represents an Internet Protocol address.
struct IPAddress
   {
   public:
      /// Initializes an `IPAddress` with the address specified as an integer value.
      /// \param address The integer value of the IP address in network byte order.
      explicit constexpr IPAddress( uint32_t address ) noexcept: _address( address )
         { }

      /// Gets the integer value of the IP address.
      /// \return The integer value of the IP address in network byte order.
      [[nodiscard]] constexpr uint32_t address( ) const noexcept
         { return _address; }

      static const IPAddress any; /// Indicates that the server must listen for client activity on all network interfaces.
      static const IPAddress broadcast; ///< Represents the IP broadcast address.
      static const IPAddress loopBack; ///< Represents the IP loopback address.

      /// Tries to convert an IP address string to an `IPAddress`.
      /// \param ipString An IP address string.
      /// \return An `IPAddress` if `ipString` was able to be parsed as an IP address.
      static std::optional<IPAddress> tryParse( StringView ipString ) noexcept
         {
         uint32_t address;
         if ( inet_pton( AF_INET, ipString.data( ), &address ) == 1 )
            return IPAddress( address );
         return std::nullopt;
         }

      /// Converts a short value from host byte order to network byte order.
      /// \param host The number to convert.
      /// \return A short value expressed in network byte order.
      static uint16_t hostToNetworkOrder( uint16_t host ) noexcept
         { return htons( host ); }

      /// Converts an integer value from host byte order to network byte order.
      /// \param host The number to convert.
      /// \return An integer value expressed in network byte order.
      static uint32_t hostToNetworkOrder( uint32_t host ) noexcept
         { return htonl( host ); }

      /// Converts a short value from network byte order to host byte order.
      /// \param network The number to convert.
      /// \return A short value expressed in host byte order.
      static uint16_t networkToHostOrder( uint16_t network ) noexcept
         { return ntohs( network ); }

      /// Converts an integer value from network byte order to host byte order.
      /// \param network The number to convert.
      /// \return An integer value expressed in host byte order.
      static uint32_t networkToHostOrder( uint32_t network ) noexcept
         { return ntohs( network ); }

      constexpr bool operator==( const IPAddress &rhs ) const noexcept
         { return _address == rhs._address; }
      constexpr bool operator!=( const IPAddress &rhs ) const noexcept
         { return !( rhs == *this ); }

      friend std::ostream &operator<<( std::ostream &stream, IPAddress ipAddress )
         {
         String str( INET_ADDRSTRLEN, '\0' );
         inet_ntop( AF_INET, &ipAddress._address, str.data( ), INET_ADDRSTRLEN );
         return stream << str.data( );
         }

   private:
      uint32_t _address;
   };

inline const IPAddress IPAddress::any( IPAddress::hostToNetworkOrder( INADDR_ANY ) );
inline const IPAddress IPAddress::broadcast( IPAddress::hostToNetworkOrder( INADDR_BROADCAST ) );
inline const IPAddress IPAddress::loopBack( IPAddress::hostToNetworkOrder( INADDR_LOOPBACK ) );

/// Identifies a network address.
struct EndPoint
   {
   public:
      virtual ~EndPoint( ) noexcept = default;

   protected:
      EndPoint( ) noexcept = default;
   };

/// Represents a network endpoint as a host name or an IP address string and a port number.
struct DnsEndPoint : public EndPoint
   {
   public:
      std::string host; ///< The host name or IP address string.
      int port; ///< The port number in host order.

      /// Initializes a `DnsEndPoint` with a specified host name or an IP address string and a port number.
      /// \param host The host name or IP address string.
      /// \param port The port number in host order.
      DnsEndPoint( String host, int port ) : host( std::move( host ) ), port( port )
         { }

      bool operator==( const DnsEndPoint &rhs ) const noexcept
         { return std::tie( host, port ) == std::tie( rhs.host, rhs.port ); }
      bool operator!=( const DnsEndPoint &rhs ) const noexcept
         { return !( rhs == *this ); }

      friend std::ostream &operator<<( std::ostream &stream, const DnsEndPoint &endPoint )
         { return stream << endPoint.host << ":" << endPoint.port; }
   };

/// Represents a network endpoint as an IP address and a port number.
struct IPEndPoint : public EndPoint
   {
   public:
      IPAddress address; ///< The IP address.
      int port; /// < The port number in host order.

      /// Initializes an `IPEndPoint` with a specified IP address and a port number.
      /// \param address The IP address.
      /// \param port The port number in host order.
      IPEndPoint( IPAddress address, int port ) noexcept: address( address ), port( port )
         { }

      /// Initializes an `IPEndPoint` with a specified IP address and a port number.
      /// \param address The integer value of the IP address in network order.
      /// \param port The port number in host order.
      IPEndPoint( uint32_t address, int port ) noexcept: address( address ), port( port )
         { }

      /// Creates an `IPEndPoint` from a socket address.
      /// \param socketAddress The socket address.
      /// \return An `IPEndPoint` using the specified socket address.
      static IPEndPoint create( const SocketAddress &socketAddress ) noexcept
         {
         const auto &internetSocketAddress = reinterpret_cast<const sockaddr_in &>(socketAddress);
         return IPEndPoint( internetSocketAddress.sin_addr.s_addr,
                            IPAddress::networkToHostOrder( internetSocketAddress.sin_port ) );
         }

      /// Serializes `IPEndPoint` information into `SocketAddress`.
      /// \return A `SocketAddress` containing the socket address for the `IPEndPoint`.
      [[nodiscard]] SocketAddress serialize( ) const
         {
         SocketAddress socketAddress{ };
         *reinterpret_cast<sockaddr_in *>(&socketAddress) = sockaddr_in{
               .sin_family = static_cast<sa_family_t>(AddressFamily::InterNetwork),
               .sin_port = IPAddress::hostToNetworkOrder( static_cast<uint16_t>(port) ),
               .sin_addr = { .s_addr = address.address( ) }
         };
         return socketAddress;
         }

      bool operator==( const IPEndPoint &rhs ) const noexcept
         { return std::tie( address, port ) == std::tie( rhs.address, rhs.port ); }
      bool operator!=( const IPEndPoint &rhs ) const noexcept
         { return !( rhs == *this ); }

      friend std::ostream &operator<<( std::ostream &stream, const IPEndPoint &endPoint )
         { return stream << endPoint.address << ":" << endPoint.port; };
   };

/// Provides domain name resolution functionality.
class Dns
   {
   public:
      Dns( ) = delete;

      /// Gets the IP addresses for the specified host.
      /// \param hostNameOrAddress The host name or IP address to resolve.
      /// \return The IP addresses for the specified host.
      /// \throw SocketException An error is encountered when resolving `hostNameOrAddress`.
      [[nodiscard]] static Vector<IPAddress> getHostAddresses( StringView hostNameOrAddress );
   };

/// Implements the Berkeley sockets interface.
class Socket
   {
   public:
      /// Initializes a `Socket` using the specified address family, socket type and protocol.
      /// \param addressFamily The address family.
      /// \param socketType The socket type.
      /// \param protocolType The protocol type.
      /// \throw SocketException A socket error occurred.
      Socket( AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType );

      Socket( const Socket & ) = delete;
      Socket &operator=( const Socket & ) = delete;

      Socket( Socket &&other ) noexcept;
      Socket &operator=( Socket &&other ) noexcept;

      ~Socket( )
         { if ( _handle != -1 ) close( ); }

      /// Gets the address family.
      /// \return The address family.
      [[nodiscard]] AddressFamily addressFamily( ) const noexcept
         { return _addressFamily; }

      /// Gets the socket type.
      /// \return The socket type.
      [[nodiscard]] SocketType socketType( ) const noexcept
         { return _socketType; }

      /// Gets the protocol type.
      /// \return The protocol type.
      [[nodiscard]] ProtocolType protocolType( ) const noexcept
         { return _protocolType; }

      /// Gets the operating system handle for the socket.
      /// \return The operating system handle for the socket.
      [[nodiscard]] int handle( ) const noexcept
         { return _handle; }

      /// Gets the local endpoint if it exists.
      /// \return The local endpoint if it exists.
      [[nodiscard]] const std::optional<IPEndPoint> &localEndPoint( ) const noexcept
         { return _localEP; }

      /// Gets the remote endpoint if it exists.
      /// \return The remote endpoint if it exists.
      [[nodiscard]] const std::optional<IPEndPoint> &remoteEndPoint( ) const noexcept
         { return _remoteEP; }

      /// Gets the amount of data available to read.
      /// \return The number of bytes of data available to read.
      /// \throw SocketException A socket error occurred.
      [[nodiscard]] int available( ) const;

      /// Sets the timeout for sending.
      /// \param value The timeout value in seconds.
      /// \throw SocketException A socket error occurred.
      void setSendTimeout( int value ) const
         { setSocketOption( SocketOptionLevel::Socket, SocketOptionName::SendTimeout, timeval{ .tv_sec = value } ); }

      /// Sets the timeout for receiving.
      /// \param value The timeout value in seconds.
      /// \throw SocketException A socket error occurred.
      void setReceiveTimeout( int value ) const
         { setSocketOption( SocketOptionLevel::Socket, SocketOptionName::ReceiveTimeout, timeval{ .tv_sec = value } ); }

      /// Sets the specified socket option to the specified boolean value.
      /// \param optionLevel The socket option level.
      /// \param optionName The socket option name.
      /// \param optionValue The option value.
      /// \throw SocketException A socket error occurred.
      void setSocketOption( SocketOptionLevel optionLevel, SocketOptionName optionName, bool optionValue ) const
         { setSocketOption( optionLevel, optionName, static_cast<int>(optionValue) ); }

      /// Sets the specified socket option to the specified value.
      /// \param optionLevel The socket option level.
      /// \param optionName The socket option name.
      /// \param optionValue The option value.
      /// \throw SocketException A socket error occurred.
      template<typename T>
      void setSocketOption( SocketOptionLevel optionLevel, SocketOptionName optionName, const T &optionValue ) const
         {
         if ( setsockopt( _handle, static_cast<int>(optionLevel), static_cast<int>(optionName),
                          &optionValue, sizeof( T ) ) == -1 )
            throw SocketException( );
         }

      /// Associates the socket with a local endpoint.
      /// \param localEP The local endpoint to associate the socket with.
      /// \throw SocketException A socket error occurred.
      void bind( const IPEndPoint &localEP );

      /// Places the socket in a listening state.
      /// \param backlog The maximum length of the pending connection queue.
      /// \throw SocketException A socket error occurred.
      void listen( int backlog ) const;

      /// Establishes a connection to a remote host specified by a host name and a port number.
      /// \param host The host name of the remote host.
      /// \param port The port number of the remote host.
      /// \throw SocketException A socket error occurred.
      void connect( StringView host, int port );

      /// Establishes a connection to a remote host specified by an IP address and a port number.
      /// \param address The IP address of the remote host.
      /// \param port The port number of the remote host.
      /// \throw SocketException A socket error occurred.
      void connect( IPAddress address, int port );

      /// Establishes a connection to a remote host.
      /// \param remoteEP The remote host.
      /// \throw SocketException A socket error occurred.
      void connect( const EndPoint &remoteEP );

      /// Creates a new `Socket` for a newly created connection.
      /// \return The `Socket` for the newly created connection.
      /// \throw SocketException A socket error occurred.
      Socket accept( );

      /// Sends the specified number of bytes to a connected socket using the specified `SocketFlags`.
      /// \param buffer The data to send.
      /// \param count The number of bytes to send.
      /// \param socketFlags A bitwise combination of `SocketFlags` values.
      /// \return The actual number of bytes sent.
      /// \throw SocketException A socket error occurred.
      int send( const std::byte *buffer, int count, SocketFlags socketFlags = SocketFlags::None ) const;

      /// Sends the specified number of bytes to the specified endpoint using the specified `SocketFlags`.
      /// \param buffer The data to send.
      /// \param count The number of bytes to send.
      /// \param remoteEP The destination location for the data.
      /// \param socketFlags A bitwise combination of `SocketFlags` values.
      /// \return The actual number of bytes sent.
      /// \throw SocketException A socket error occurred.
      int sendTo( const std::byte *buffer, int count, const IPEndPoint &remoteEP,
                  SocketFlags socketFlags = SocketFlags::None );

      /// Receives the specified number of bytes from a bound socket into the specified buffer using the specified `SocketFlags`.
      /// \param buffer The storage location for the received data.
      /// \param size The number of bytes to receive.
      /// \param socketFlags A bitwise combination of `SocketFlags` values.
      /// \return The actual number of bytes received.
      /// \throw SocketException A socket error occurred.
      int receive( std::byte *buffer, int count, SocketFlags socketFlags = SocketFlags::None ) const;

      /// Receives the specified number of bytes into the specified buffer using the specified `SocketFlags`, and stores
      /// the remote endpoint.
      /// \param buffer The storage location for the received data.
      /// \param size The number of bytes to receive.
      /// \param remoteEP The remote endpoint.
      /// \param socketFlags A bitwise combination of `SocketFlags` values.
      /// \return The actual number of bytes received.
      /// \throw SocketException A socket error occurred.
      int receiveFrom( std::byte *buffer, int count, IPEndPoint *remoteEP,
                       SocketFlags socketFlags = SocketFlags::None );

      /// Closes the socket connection.
      /// \throw SocketException A socket error occurred.
      void close( );

   private:
      Socket( AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType, int handle ) noexcept:
            _addressFamily( addressFamily ), _socketType( socketType ), _protocolType( protocolType ), _handle( handle )
         { }

      AddressFamily _addressFamily;
      SocketType _socketType;
      ProtocolType _protocolType;
      int _handle = -1;
      std::optional<IPEndPoint> _localEP;
      std::optional<IPEndPoint> _remoteEP;
   };
