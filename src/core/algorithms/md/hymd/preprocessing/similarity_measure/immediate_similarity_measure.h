#pragma once

#include "model/types/double_type.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/sim_measure.h"

namespace algos::hymd::preprocessing::similarity_measure {

class ImmediateSimilarityMeasure final : public SimilarityMeasure {
    [[nodiscard]] indexes::ColumnMatchSimilarityInfo MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<indexes::PliCluster> const* clusters_right,
            model::md::DecisionBoundary min_sim, bool is_null_equal_null) const final;
public:
    ImmediateSimilarityMeasure(std::string name, std::unique_ptr<model::Type> arg_type,
                               SimilarityFunction compute_similarity)
        : SimilarityMeasure(std::move(arg_type),std::make_unique<model::DoubleType>(), std::move(compute_similarity)){};
};

}  // namespace algos::hymd::preprocessing::similarity_measure