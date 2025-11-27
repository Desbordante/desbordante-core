#pragma once

#include <string>

#include "core/algorithms/ind/faida/hashing/murmur_hash_3.h"

namespace algos::faida::hashing {

inline size_t CalcMurmurHash(std::string const& str) {
    size_t hash_long[2];
    unsigned constexpr seed = 0;
    MurmurHash3_x64_128(str.data(), str.size(), seed, &hash_long);
    return hash_long[0];
}

}  // namespace algos::faida::hashing
