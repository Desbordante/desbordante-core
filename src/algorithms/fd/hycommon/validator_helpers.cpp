#include "validator_helpers.h"

#include "algorithms/fd/hycommon/util/pli_util.h"
#include "algorithms/fd/hyfd/structures/fd_tree_vertex.h"
#include "ucc/hyucc/structures/ucc_tree_vertex.h"

namespace algos::hy {

std::vector<ClusterId> BuildClustersIdentifier(Row const& compressed_record,
                                               std::vector<ClusterId> const& agree_set) {
    std::vector<ClusterId> sub_cluster;
    sub_cluster.reserve(agree_set.size());
    for (auto attr : agree_set) {
        ClusterId const cluster_id = compressed_record[attr];

        if (PLIUtil::IsSingletonCluster(cluster_id)) {
            return {};
        }

        sub_cluster.push_back(cluster_id);
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

using UCCLhsPair = algos::hyucc::LhsPair;
using FDLhsPair = algos::hyfd::fd_tree::LhsPair;
template std::vector<UCCLhsPair> CollectCurrentChildren<UCCLhsPair>(
        std::vector<UCCLhsPair> const& cur_level_vertices, size_t num_attributes);
template std::vector<FDLhsPair> CollectCurrentChildren<FDLhsPair>(
        std::vector<FDLhsPair> const& cur_level_vertices, size_t num_attributes);

}  // namespace algos::hy
