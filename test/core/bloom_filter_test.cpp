#include <gtest/gtest.h>

#include "core/hash_table/bloom_filter.h"

TEST( BloomFilterTest, Basic )
   {
   static constexpr auto size = 100;
   BloomFilter<int> bloomFilter( 1000, 1e-3 );
   for ( auto i = 0; i < size; ++i )
      EXPECT_FALSE( bloomFilter.contains( i ) );
   for ( auto i = 0; i < size; ++i ) bloomFilter.insert( i );
   for ( auto i = 0; i < size; ++i )
      EXPECT_TRUE( bloomFilter.contains( i ) );
   }

TEST( BloomFilterTest, StreamOperators )
   {
   static constexpr auto size = 100;
   BloomFilter<int> bloomFilter( 1000, 1e-3 );
   for ( auto i = 0; i < size; ++i ) bloomFilter.insert( i );

   std::stringstream stream;
   stream << bloomFilter;

   bloomFilter.clear( );
   for ( auto i = 0; i < size; ++i )
      EXPECT_FALSE( bloomFilter.contains( i ) );

   stream >> bloomFilter;
   for ( auto i = 0; i < size; ++i )
      EXPECT_TRUE( bloomFilter.contains( i ) );
   }
