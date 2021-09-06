#pragma once

#include <chrono>

inline auto putCurrentDateTime( )
   {
   const auto time = std::time( nullptr );
   return std::put_time( std::localtime( &time ), "%c" );
   }
