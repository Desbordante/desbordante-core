#pragma once

#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "model/types/double_type.h"

namespace algos::hymd::preprocessing::similarity_measure {
constexpr model::md::DecisionBoundary kLowestBound = 0.0;
using SimilarityFunction = std::function<Similarity(std::byte const*, std::byte const*)>;

class ImmediateSimilarityMeasure : public SimilarityMeasure {
private:
    SimilarityFunction const compute_similarity_;

    [[nodiscard]] indexes::ColumnMatchSimilarityInfo MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<indexes::PliCluster> const& clusters_right) const final;

public:
    ImmediateSimilarityMeasure(std::unique_ptr<model::Type> arg_type,
                               SimilarityFunction compute_similarity)
        : SimilarityMeasure(std::move(arg_type), std::make_unique<model::DoubleType>()),
          compute_similarity_(std::move(compute_similarity)){};
};

}  // namespace algos::hymd::preprocessing::similarity_measure