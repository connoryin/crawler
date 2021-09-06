#include <gtest/gtest.h>

#include "core/concurrency/thread.h"
#include "core/memory.h"

//TEST( ThreadTest, Name )
//   {
//   Thread::CurrentThread::setName( "Thread-Test" );
//   EXPECT_EQ( Thread::CurrentThread::name( ), "Thread-Test" );
//   }

TEST( ThreadTest, RValueArgs )
   {
   auto value = makeUnique<int>( 1024 );
   Thread thread( [ ]( UniquePtr<int> value )
                     { EXPECT_EQ( *value, 1024 ); },
                  std::move( value ) );
   thread.join( );
   }
