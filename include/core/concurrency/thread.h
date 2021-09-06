#pragma once

#include <pthread.h>
#include <tuple>
#include <unistd.h>
#include <thread>

#include "core/exception.h"
#include "core/string.h"

using Thread = std::thread;

/// Creates and controls a thread.
//class Thread
//   {
//   public:
//      /// Controls the currently running thread.
//      class CurrentThread
//         {
//         public:
//            CurrentThread( ) = delete;
//
//            /// Gets the name of the current thread.
//            /// \return The name of the current thread.
//            /// \throw SystemException A system error occurred.
//            [[nodiscard]] static String name( )
//               {
//               std::array<char, 16> buffer{ };
//               const auto errorCode = pthread_getname_np( pthread_self( ), buffer.data( ), buffer.size( ) );
//               if ( errorCode != 0 ) throw SystemException( errorCode );
//               return buffer.data( );
//               }
//
//            /// Sets the name of the current thread.
//            /// \param value The name of the current thread.
//            /// \throw SystemException A system error occurred.
//            static void setName( StringView value )
//               {
//               const auto errorCode = pthread_setname_np( pthread_self( ), value.data( ) );
//               if ( errorCode != 0 ) throw SystemException( errorCode );
//               }
//
//            /// Suspends the current thread for the specified number of seconds.
//            /// \param secondsTimeout The number of seconds for which the current thread is suspended.
//            static void sleep( int secondsTimeout )
//               { ::sleep( secondsTimeout ); }
//         };
//
//      /// Initializes a `Thread` that does not represent a thread.
//      Thread( ) noexcept = default;
//
//      /// Initializes a `Thread` with the specified function and arguments.
//      /// \param f The function to invoke upon execution.
//      /// \param args The arguments to pass to the function.
//      /// \throw SystemException A system error occurred.
//      template<typename Function, typename ... Args>
//      explicit Thread( Function f, Args &&...args )
//         {
//         auto *const packed = new std::pair( f, std::forward_as_tuple( std::forward<Args>( args )... ) );
//         using PackedFunction = decltype( packed );
//         const auto wrapped = [ ]( void *arg ) -> void *
//            {
//            auto *const packed = reinterpret_cast<PackedFunction>(arg);
//            std::apply( packed->first, std::move( packed->second ) );
//            delete packed;
//            return nullptr;
//            };
//         const auto errorCode = pthread_create( &_handle, nullptr, wrapped, reinterpret_cast<void *>(packed) );
//         if ( errorCode != 0 ) throw SystemException( errorCode );
//         _joinable = true;
//         }
//
//      Thread( const Thread & ) = delete;
//      Thread &operator=( const Thread & ) = delete;
//
//      Thread( Thread &&other ) noexcept:
//            _handle( std::exchange( other._handle, 0 ) ),
//            _joinable( std::exchange( other._joinable, false ) )
//         { }
//      Thread &operator=( Thread &&other ) noexcept
//         {
//         if ( this != &other )
//            {
//            if ( _joinable ) std::terminate( );
//            _handle = std::exchange( other._handle, 0 );
//            _joinable = std::exchange( other._joinable, false );
//            }
//         return *this;
//         }
//
//      ~Thread( )
//         { if ( _joinable ) std::terminate( ); }
//
//      /// Indicates whether the `Thread` represents an active thread of execution.
//      /// \return `true` if the `Thread` represents an active thread of execution.
//      [[nodiscard]] bool joinable( ) const noexcept
//         { return _joinable; }
//
//      /// Gets the operating system handle for the `Thread`.
//      /// \return The operating system handle for the `Thread`.
//      [[nodiscard]] pthread_t handle( ) const noexcept
//         { return _handle; }
//
//      /// Gets the name of the `Thread`.
//      /// \return The name of the `Thread`.
//      /// \throw SystemException A system error occurred.
//      [[nodiscard]] String name( ) const
//         {
//         std::array<char, 16> buffer{ };
//         const auto errorCode = pthread_getname_np( _handle, buffer.data( ), buffer.size( ) );
//         if ( errorCode != 0 ) throw SystemException( errorCode );
//         return buffer.data( );
//         }
//
//      /// Sets the name of the `Thread`.
//      /// \param value The name of the `Thread`.
//      /// \throw SystemException A system error occurred.
//      void setName( StringView value ) const
//         {
//         const auto errorCode = pthread_setname_np( _handle, value.data( ) );
//         if ( errorCode != 0 ) throw SystemException( errorCode );
//         }
//
//      /// Blocks the calling thread until the `Thread` terminates.
//      /// \throw InvalidOperationException The `Thread` is not joinable.
//      /// \throw SystemException A system error occurred.
//      void join( )
//         {
//         if ( !_joinable ) throw InvalidOperationException( "The thread is not joinable." );
//         const auto errorCode = pthread_join( _handle, nullptr );
//         if ( errorCode != 0 ) throw SystemException( errorCode );
//         _joinable = false;
//         }
//
//      /// Separates the thread of execution from the `Thread` object, which allows execution to continue independently.
//      /// \throw InvalidOperationException The `Thread` is not joinable.
//      /// \throw SystemException A system error occurred.
//      void detach( )
//         {
//         if ( !_joinable ) throw InvalidOperationException( "The thread is not joinable." );
//         const auto errorCode = pthread_detach( _handle );
//         if ( errorCode != 0 ) throw SystemException( errorCode );
//         _joinable = false;
//         }
//
//   private:
//      pthread_t _handle = 0;
//      bool _joinable = false;
//   };
