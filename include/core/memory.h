#pragma once

#include <memory>

template<typename T, typename Deleter = std::default_delete<T>>
using UniquePtr = std::unique_ptr<T, Deleter>;

template<typename T, typename... Args>
UniquePtr<T> makeUnique( Args &&... args )
   { return UniquePtr<T>( new T( std::forward<Args>( args )... ) ); }
