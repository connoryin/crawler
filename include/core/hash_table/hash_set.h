#pragma once

#include <unordered_set>

#include "core/hash_table/hash.h"

template<typename T>
using HashSet = std::unordered_set<T, Hash<T>>;
