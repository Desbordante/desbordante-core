#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include <boost/container_hash/hash.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/version.hpp>
#include <easylogging++.h>

#include "types.h"

#define UNORDERED_FLAT_MAP_AVAILABLE (BOOST_VERSION >= 108100)

#if UNORDERED_FLAT_MAP_AVAILABLE
#include <boost/unordered/unordered_flat_map.hpp>
#endif

namespace algos::hy {

// Builds a cluster's identifier of the agree set provided. Cluster's identifier is a vector
// of size_t value where ith value of the vector is an identifier of a cluster of ith set
// attribute of the agree set.
std::vector<ClusterId> BuildClustersIdentifier(Row const& compressed_record,
                                               std::vector<ClusterId> const& agree_set);

// Builds the next level of the prefix tree traversal
template <typename VertexAndAgreeSet>
std::vector<VertexAndAgreeSet> CollectCurrentChildren(
        std::vector<VertexAndAgreeSet> const& cur_level_vertices, size_t num_attributes);

template <typename VertexAndAgreeSet, typename InstanceValidations>
void LogLevel(const std::vector<VertexAndAgreeSet>& cur_level_vertices,
              const InstanceValidations& result, size_t candidates, size_t current_level_number,
              std::string_view primitive) {
    int const num_invalid_instances = result.invalid_instances().size();
    int const num_valid_instances = result.count_validations() - num_invalid_instances;

    LOG(INFO) << "LEVEL " << current_level_number << "(" << cur_level_vertices.size()
              << "): " << result.count_intersections() << " intersections; "
              << result.count_validations() << " validations; " << num_invalid_instances
              << " invalid; " << candidates << " new candidates; --> " << num_valid_instances << " "
              << primitive << "s";
}

template <typename T>
auto MakeClusterIdentifierToTMap(size_t bucket_size) {
    auto const kHasher = [](std::vector<ClusterId> const& v) noexcept {
        size_t hash = 1;
        for (auto it = v.rbegin(); it != v.rend(); ++it) {
            hash = 31 * hash + *it;
        }
        return hash;
    };

#if UNORDERED_FLAT_MAP_AVAILABLE
    using UnorderedMap = boost::unordered_flat_map<std::vector<ClusterId>, T, decltype(kHasher)>;
#else
    using UnorderedMap = std::unordered_map<std::vector<ClusterId>, T, decltype(kHasher)>;
#endif
    return UnorderedMap(bucket_size, kHasher);
}

#undef UNORDERED_FLAT_MAP_AVAILABLE

}  // namespace algos::hy
