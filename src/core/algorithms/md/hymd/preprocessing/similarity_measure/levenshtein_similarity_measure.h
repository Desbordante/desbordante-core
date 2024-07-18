#pragma once

#include <memory>

#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "algorithms/md/hymd/similarity_measure_creator.h"

namespace algos::hymd::preprocessing::similarity_measure {

class LevenshteinSimilarityMeasure final : public SimilarityMeasure {
    model::md::DecisionBoundary const min_sim_;
    std::size_t const size_limit_;
    util::WorkerThreadPool* const pool_;
    static constexpr auto kName = "levenshtein_similarity";

public:
    class Creator final : public SimilarityMeasureCreator {
        model::md::DecisionBoundary const min_sim_;
        std::size_t const size_limit_;

    public:
        Creator(model::md::DecisionBoundary min_sim = 0.7, std::size_t size_limit = 0);

        std::unique_ptr<SimilarityMeasure> MakeMeasure(
                util::WorkerThreadPool* thread_pool) const final {
            return std::make_unique<LevenshteinSimilarityMeasure>(min_sim_, size_limit_,
                                                                  thread_pool);
        }
    };

    [[nodiscard]] indexes::SimilarityMeasureOutput MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<indexes::PliCluster> const& clusters_right) const final;

    LevenshteinSimilarityMeasure(model::md::DecisionBoundary min_sim, std::size_t size_limit,
                                 util::WorkerThreadPool* thread_pool);
};

}  // namespace algos::hymd::preprocessing::similarity_measure
