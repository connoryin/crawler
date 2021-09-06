#pragma once

#include <functional>

template<typename T>
struct Hash
   {
   public:
      size_t operator()( const T &value ) const
         { return std::hash<T>( )( value ); }
   };
