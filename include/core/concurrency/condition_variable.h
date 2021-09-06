#pragma once

#include <pthread.h>

#include "core/concurrency/mutex.h"

class ConditionVariable
   {
   public:
      ConditionVariable( ) noexcept
         { pthread_cond_init( &_handle, nullptr ); }

      ConditionVariable( const ConditionVariable & ) = delete;
      ConditionVariable &operator=( const ConditionVariable & ) = delete;
      ConditionVariable( ConditionVariable && ) = delete;
      ConditionVariable &operator=( ConditionVariable && ) = delete;

      ~ConditionVariable( ) noexcept
         { pthread_cond_destroy( &_handle ); }

      template<typename Mutex>
      void wait( UniqueLock<Mutex> &lock )
         { pthread_cond_wait( &_handle, &lock.mutex( ).handle( ) ); }

      template<typename Mutex, typename Predicate>
      void wait( UniqueLock<Mutex> &lock, Predicate predicate )
         { while ( !predicate( ) ) wait( lock ); }

      void notifyOne( ) noexcept
         { pthread_cond_signal( &_handle ); }

      void notifyAll( ) noexcept
         { pthread_cond_broadcast( &_handle ); }

      pthread_cond_t &handle( ) noexcept
         { return _handle; }

   private:
      pthread_cond_t _handle;
   };
