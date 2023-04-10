#include "validator_helpers.h"

#include "hyfd/util/pli_util.h"

namespace algos {

std::vector<size_t> BuildClustersIdentifier(std::vector<size_t> const& compressed_record,
                                            std::vector<size_t> const& agree_set) {
    std::vector<size_t> sub_cluster;
    sub_cluster.reserve(agree_set.size());
    for (size_t attr : agree_set) {
        size_t const cluster_id = compressed_record[attr];

        if (!algos::hyfd::PLIUtil::IsSingletonCluster(cluster_id)) {
            sub_cluster.push_back(cluster_id);
        } else {
            return {};
        }
    }
    return sub_cluster;
}

}  // namespace algos
