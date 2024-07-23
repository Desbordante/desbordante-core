#pragma once

#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/ccv_id_pickers/index_uniform.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "model/types/double_type.h"

namespace algos::hymd::preprocessing::similarity_measure {
using DistanceFunction = std::function<size_t(std::byte const*, std::byte const*)>;

class DistanceSimilarityMeasure : public SimilarityMeasure {
private:
    DistanceFunction const compute_distance_;
    model::md::DecisionBoundary min_sim_;
    util::WorkerThreadPool* const pool_;
    std::size_t const size_limit_;
    ccv_id_pickers::IndexUniform picker_{size_limit_};

    [[nodiscard]] indexes::SimilarityMeasureOutput MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<indexes::PliCluster> const& clusters_right) const final;

public:
    DistanceSimilarityMeasure(std::unique_ptr<model::Type> arg_type,
                              DistanceFunction compute_distance,
                              model::md::DecisionBoundary min_sim,
                              util::WorkerThreadPool* thread_pool, std::size_t size_limit)
        : SimilarityMeasure(std::move(arg_type), std::make_unique<model::DoubleType>()),
          compute_distance_(std::move(compute_distance)),
          min_sim_(min_sim),
          pool_(thread_pool),
          size_limit_(size_limit){};
};

}  // namespace algos::hymd::preprocessing::similarity_measure
