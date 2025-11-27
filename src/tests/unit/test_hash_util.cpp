/** \file
 * \brief Hash functions
 *
 * Helper functions to calculate hash definition
 */
#include "tests/unit/test_hash_util.h"

#include <vector>

namespace tests {

// Implement custom hash functions since implementation of `std::hash` or `boost::hash` may change
// depending on the library version/architecture/os/whatever leading to tests failing.
// Taken from
// https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector/72073933#72073933
std::size_t Hash(std::vector<unsigned> const& vec) {
    std::size_t seed = vec.size();
    for (auto x : vec) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

std::size_t Hash(std::vector<std::vector<unsigned>> const& vec) {
    return HashVec(vec, Hash);
}

std::size_t Hash(std::pair<unsigned, std::vector<unsigned>> const& pair) {
    auto const& [lhs, rhs] = pair;
    return 31 * lhs + Hash(rhs);
}

}  // namespace tests
