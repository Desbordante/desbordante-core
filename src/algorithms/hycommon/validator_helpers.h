#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include <boost/container_hash/hash.hpp>
#include <boost/dynamic_bitset.hpp>
#include <easylogging++.h>

#include "types.h"

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
    auto const kHasher = boost::hash<std::vector<ClusterId>>();
    return std::unordered_map<std::vector<ClusterId>, T, decltype(kHasher)>(bucket_size, kHasher);
}

}  // namespace algos::hy
