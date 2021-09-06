#pragma once

#include <filesystem>
#include <iomanip>

#include "core/string.h"

inline String fileSizeToString( int numBytes )
   {
   static constexpr std::array suffixes = { "B", "KB", "MB", "GB", "TB" };
   auto size = static_cast<float>(numBytes);
   auto it = suffixes.cbegin( );
   for ( ; size > 1024; ++it ) size /= 1024;
   return STRING( std::setprecision( 3 ) << size << " " << *it );
   }
