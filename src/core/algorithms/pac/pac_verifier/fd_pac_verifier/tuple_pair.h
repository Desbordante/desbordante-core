#pragma once

#include <cstddef>

namespace algos::pac_verifier {
/// @brief Holds info about pair of tuples: indices of tuples and distance between them.
struct TuplePair {
    std::size_t first_idx;
    std::size_t second_idx;
    double rhs_dist;
};
}  // namespace algos::pac_verifier
