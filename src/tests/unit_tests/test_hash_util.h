/** \file
 * \brief Hash functions
 *
 * Helper functions to calculate hash
 */
#pragma once

#include <utility>
#include <vector>

namespace tests {

std::size_t Hash(std::vector<unsigned> const& vec);
std::size_t Hash(std::vector<std::vector<unsigned>> const& vec);
std::size_t Hash(std::pair<unsigned, std::vector<unsigned>> const& pair);

template <typename T>
std::size_t HashVec(std::vector<T> const& vec, std::size_t (*hash_fn)(T const&)) {
    std::size_t hash = 1;
    for (auto const& v : vec) {
        hash = 31 * hash + hash_fn(v);
    }
    return hash;
}

template <typename T>
std::size_t HashPair(std::pair<T, T> const& pair) {
    auto const& [lhs, rhs] = pair;
    std::size_t lhs_hash = Hash(lhs);
    std::size_t rhs_hash = Hash(rhs);
    return lhs_hash ^ (rhs_hash + 0x9e3779b9 + (lhs_hash << 6) + (lhs_hash >> 2));
}

}  // namespace tests
