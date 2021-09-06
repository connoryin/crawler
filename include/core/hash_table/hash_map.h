#pragma once

#include <unordered_map>

#include "core/hash_table/hash.h"

template<typename Key, typename T>
using HashMap = std::unordered_map<Key, T, Hash<Key>>;
