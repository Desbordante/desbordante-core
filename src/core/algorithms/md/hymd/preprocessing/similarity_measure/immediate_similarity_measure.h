#pragma once

#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/ccv_id_pickers/index_uniform.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "model/types/double_type.h"

namespace algos::hymd::preprocessing::similarity_measure {
using SimilarityFunction = std::function<Similarity(std::byte const*, std::byte const*)>;

class ImmediateSimilarityMeasure : public SimilarityMeasure {
private:
    SimilarityFunction const compute_similarity_;
    util::WorkerThreadPool* const pool_;
    std::size_t const size_limit_;
    ccv_id_pickers::IndexUniform picker_{size_limit_};

    [[nodiscard]] indexes::SimilarityMeasureOutput MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<indexes::PliCluster> const& clusters_right) const final;

public:
    ImmediateSimilarityMeasure(std::unique_ptr<model::Type> arg_type,
                               bool is_symmetrical_and_eq_is_max,
                               SimilarityFunction compute_similarity,
                               util::WorkerThreadPool* thread_pool, std::size_t size_limit)
        : SimilarityMeasure(std::move(arg_type), std::make_unique<model::DoubleType>(),
                            is_symmetrical_and_eq_is_max),
          compute_similarity_(std::move(compute_similarity)),
          pool_(thread_pool),
          size_limit_(size_limit) {};
};

}  // namespace algos::hymd::preprocessing::similarity_measure
