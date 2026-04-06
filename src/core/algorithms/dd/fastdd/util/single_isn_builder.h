#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <boost/unordered/unordered_flat_set.hpp>

#include "core/algorithms/dd/fastdd/model/pli_shard.h"
#include "core/algorithms/dd/fastdd/util/distance_calculator.h"
#include "core/algorithms/dd/fastdd/util/isn_info.h"

namespace algos::dd {

class SingleISNBuilder {
private:
    std::shared_ptr<ISNInfo> isn_info_;
    std::shared_ptr<DistanceCalculator> distance_calculator_;
    PliShard const& pli_shard_;
    std::vector<std::size_t> clues_;

    void BuildNotDistanceOrdered(DFPack const& df_pack);
    void BuildDistanceOrdered(DFPack const& df_pack);
    void SetNumMask(Cluster const& cluster, std::size_t const base, std::size_t const offset);
    void SetNumMask(Cluster const& first_cluster, Cluster const& second_cluster,
                    std::size_t const base, std::size_t const offset);
    std::vector<std::size_t> CalculateOffsets(DFPack const& df_pack,
                                              std::size_t const start_cluster_num) const;

public:
    SingleISNBuilder(std::shared_ptr<ISNInfo> isn_info,
                     std::shared_ptr<DistanceCalculator> distance_calculator,
                     PliShard const& pli_shard)
        : isn_info_(isn_info), distance_calculator_(distance_calculator), pli_shard_(pli_shard) {}

    boost::unordered::unordered_flat_set<std::size_t> BuildISNs();
};

}  // namespace algos::dd
