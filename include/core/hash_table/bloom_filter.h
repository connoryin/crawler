#pragma once

#include <climits>
#include <cmath>
#include <cstddef>

#include "core/hash_table/hash.h"
#include "core/io.h"
#include "core/string.h"
#include "core/vector.h"

/// Provides probabilistic query about elements membership.
/// \tparam T The type of elements.
/// \tparam Hash The type of the hash function.
template<typename T, typename Hash = Hash<T>>
class BloomFilter
   {
   public:
      /// Initializes a `BloomFilter` with the specified expected size and false positive rate.
      /// \param expectedSize The expected size.
      /// \param falsePositiveRate The desired false positive rate.
      BloomFilter( int expectedSize, double falsePositiveRate )
         {
         const auto bitVecSize =
               std::ceil( -expectedSize * std::log( falsePositiveRate ) / ( std::log( 2 ) * std::log( 2 ) ) );
         _bitVec.resize( bitVecSize, false );
         _numHashFunctions = std::round( bitVecSize / expectedSize * std::log( 2 ) );
         }

      /// Gets the size of the `BloomFilter`.
      /// \return The size of the `BloomFilter`.
      [[nodiscard]] int size( ) const noexcept
         { return _size; }

      /// Indicates if the `BloomFilter` is empty.
      /// \return `true` if the `BloomFilter` is empty.
      [[nodiscard]] bool empty( ) const noexcept
         { return _size == 0; }

      /// Inserts a value into the `BloomFilter`.
      /// \param value The value to insert.
      void insert( const T &value )
         {
         auto[firstHashValue, secondHashValue] = getHashPair( value );
         for ( auto i = 0; i < _numHashFunctions; ++i )
            _bitVec.at( ( firstHashValue + secondHashValue * i ) % _bitVec.size( ) ) = true;
         ++_size;
         }

      /// Indicates if a value is contained in the `BloomFilter`.
      /// \param value The value to check.
      /// \return `true` indicates that the value is probably contained in the `BloomFilter`; `false` indicates that the
      /// the value is not contained in the `BloomFilter`.
      bool contains( const T &value )
         {
         auto[firstHashValue, secondHashValue] = getHashPair( value );
         for ( auto i = 0; i < _numHashFunctions; ++i )
            if ( !_bitVec.at( ( firstHashValue + secondHashValue * i ) % _bitVec.size( ) ) ) return false;
         return true;
         }

      /// Clears the `BloomFilter`.
      void clear( )
         {
         std::fill( _bitVec.begin( ), _bitVec.end( ), false );
         _size = 0;
         }

      friend std::istream &operator>>( std::istream &stream, BloomFilter &bloomFilter )
         {
         std::byte value{ };
         auto offset = -1;
         for ( size_t i = 0; i < bloomFilter._bitVec.size( ); ++i )
            {
            if ( offset == -1 )
               {
               value = static_cast<std::byte>(stream.get( ));
               offset = CHAR_BIT - 1;
               }
            bloomFilter._bitVec.at( i ) = static_cast<bool>(( value >> offset ) & std::byte{ 1 });
            }
         return stream;
         }

      friend std::ostream &operator<<( std::ostream &stream, const BloomFilter &bloomFilter )
         {
         std::byte value{ };
         auto offset = CHAR_BIT - 1;
         for ( auto bit : bloomFilter._bitVec )
            {
            value |= std::byte{ bit } << offset--;
            if ( offset == -1 )
               {
               stream.put( std::to_integer<char>( value ) );
               value = { }, offset = CHAR_BIT - 1;
               }
            }
         if ( offset < CHAR_BIT - 1 ) stream.put( std::to_integer<char>( value ) );
         return stream;
         }

   private:
      std::pair<int, int> getHashPair( const T &value )
         {
         const auto hashValue = Hash( )( value );
         return { *reinterpret_cast<const int * >(&hashValue),
                  *( reinterpret_cast<const int *>(&hashValue) + 1 ) };
         }

      Vector<bool> _bitVec;
      int _numHashFunctions;
      int _size = 0;
   };
