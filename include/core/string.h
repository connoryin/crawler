#pragma once

#include <cstring>
#include <sstream>
#include <string>

#define STRING( args ) reinterpret_cast<std::ostringstream &>(std::ostringstream( ) << args).str( )

using String = std::string;
using StringView = std::string_view;

inline char toLower( char c )
   { return static_cast<char>(std::tolower( static_cast<unsigned char>(c) )); }
