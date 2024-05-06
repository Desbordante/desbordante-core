#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "model/types/double_type.h"

namespace algos::hymd::preprocessing::similarity_measure {
using DistanceFunction = std::function<size_t(std::byte const*, std::byte const*)>;

class DistanceSimilarityMeasure : public SimilarityMeasure {
private:
    DistanceFunction const compute_distance_;
    model::md::DecisionBoundary min_sim_;

    [[nodiscard]] indexes::SimilarityMeasureOutput MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<indexes::PliCluster> const& clusters_right,
            util::WorkerThreadPool& thread_pool) const final;

public:
    DistanceSimilarityMeasure(std::unique_ptr<model::Type> arg_type,
                              DistanceFunction compute_distance,
                              model::md::DecisionBoundary min_sim)
        : SimilarityMeasure(std::move(arg_type), std::make_unique<model::DoubleType>()),
          compute_distance_(std::move(compute_distance)),
          min_sim_(min_sim){};
};

}  // namespace algos::hymd::preprocessing::similarity_measure
