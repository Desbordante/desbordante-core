#pragma once

#include <cstddef>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos {

// Builds a cluster's identifier of the agree set provided. Cluster's identifier is a vector
// of size_t value where ith value of the vector is an identifier of a cluster of ith set
// attribute of the agree set.
std::vector<size_t> BuildClustersIdentifier(std::vector<size_t> const& compressed_record,
                                            std::vector<size_t> const& agree_set);

}  // namespace algos
