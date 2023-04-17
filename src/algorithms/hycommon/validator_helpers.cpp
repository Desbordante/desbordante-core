#include "validator_helpers.h"

#include "hyfd/structures/fd_tree_vertex.h"
#include "util/pli_util.h"

namespace algos::hy {

std::vector<size_t> BuildClustersIdentifier(std::vector<size_t> const& compressed_record,
                                            std::vector<size_t> const& agree_set) {
    std::vector<size_t> sub_cluster;
    sub_cluster.reserve(agree_set.size());
    for (size_t attr : agree_set) {
        size_t const cluster_id = compressed_record[attr];

        if (!PLIUtil::IsSingletonCluster(cluster_id)) {
            sub_cluster.push_back(cluster_id);
        } else {
            return {};
        }
    }
    return sub_cluster;
}

template <typename VertexAndAgreeSet>
std::vector<VertexAndAgreeSet> CollectCurrentChildren(
        std::vector<VertexAndAgreeSet> const& cur_level_vertices, size_t num_attributes) {
    std::vector<VertexAndAgreeSet> next_level;
    for (auto const& [vertex, agree_set] : cur_level_vertices) {
        if (!vertex->HasChildren()) {
            continue;
        }

        for (size_t i = 0; i < num_attributes; ++i) {
            auto child = vertex->GetChildIfExists(i);
            if (child == nullptr) {
                continue;
            }

            boost::dynamic_bitset<> child_agree_set = agree_set;
            child_agree_set.set(i);
            next_level.emplace_back(child, std::move(child_agree_set));
        }
    }

    return next_level;
}

using FDLhsPair = algos::hyfd::fd_tree::LhsPair;
template std::vector<FDLhsPair> CollectCurrentChildren<FDLhsPair>(
        std::vector<FDLhsPair> const& cur_level_vertices, size_t num_attributes);

}  // namespace algos::hy
