#pragma once

#include <pthread.h>
#include <utility>

#ifndef NDEBUG

#include <atomic>
#include <optional>

#include "core/concurrency/thread.h"
#include "core/string.h"

#endif

class Mutex
   {
   public:
      Mutex( ) noexcept
         { pthread_mutex_init( &_handle, nullptr ); }

      Mutex( const Mutex & ) = delete;
      Mutex &operator=( const Mutex & ) = delete;
      Mutex( Mutex && ) = delete;
      Mutex &operator=( Mutex && ) = delete;

      ~Mutex( ) noexcept
         { pthread_mutex_destroy( &_handle ); }

      void lock( )
         {
#ifndef NDEBUG
         ++_queueSize;
#endif

         pthread_mutex_lock( &_handle );

#ifndef NDEBUG
         --_queueSize;
         _owner = Thread::CurrentThread::name( );
#endif
         }

      void unlock( )
         {
         pthread_mutex_unlock( &_handle );

#ifndef NDEBUG
         _owner.reset( );
#endif
         }

      pthread_mutex_t &handle( ) noexcept
         { return _handle; }

   private:
      pthread_mutex_t _handle{ };

#ifndef NDEBUG
      std::optional<String> _owner;
      std::atomic<int> _queueSize = 0;
#endif
   };

template<typename Mutex>
class UniqueLock
   {
   public:
      explicit UniqueLock( Mutex &mutex ) : _mutex( &mutex )
         { lock( ); }

      UniqueLock( const UniqueLock & ) = delete;
      UniqueLock &operator=( const UniqueLock & ) = delete;

      UniqueLock( UniqueLock &&other ) noexcept:
            _mutex( std::exchange( other._mutex, nullptr ) ), _ownsLock( std::exchange( other._ownsLock, false ) )
         { }
      UniqueLock &operator=( UniqueLock &&other ) noexcept
         {
         if ( this != &other )
            {
            if ( _ownsLock ) unlock( );
            _mutex = std::exchange( other._mutex, nullptr );
            _ownsLock = std::exchange( other._ownsLock, false );
            }
         return *this;
         }

      ~UniqueLock( )
         { if ( _ownsLock ) unlock( ); }

      void lock( )
         {
         _mutex->lock( );
         _ownsLock = true;
         }

      void unlock( )
         {
         _mutex->unlock( );
         _ownsLock = false;
         }

      Mutex &mutex( ) const noexcept
         { return *_mutex; }

      [[nodiscard]] bool ownsLock( ) const noexcept
         { return _ownsLock; }

   private:
      Mutex *_mutex = nullptr;
      bool _ownsLock = false;
   };
