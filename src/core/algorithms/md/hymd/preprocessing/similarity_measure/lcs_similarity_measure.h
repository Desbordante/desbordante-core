#pragma once

#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/lcs.h"
#include "algorithms/md/hymd/similarity_measure_creator.h"
#include "config/exceptions.h"
#include "model/types/string_type.h"

namespace algos::hymd::preprocessing::similarity_measure {

class LcsSimilarityMeasure : public ImmediateSimilarityMeasure {
    static constexpr auto kName = "lcs_similarity";

public:
    class Creator final : public SimilarityMeasureCreator {
        model::md::DecisionBoundary const min_sim_;
        std::size_t const size_limit_;

    public:
        Creator(ColumnIdentifier column1_identifier, ColumnIdentifier column2_identifier,
                model::md::DecisionBoundary min_sim = 0.7, std::size_t size_limit = 0)
            : SimilarityMeasureCreator(kName, std::move(column1_identifier),
                                       std::move(column2_identifier)),
              min_sim_(min_sim),
              size_limit_(size_limit) {
            if (!(0.0 <= min_sim_ && min_sim_ <= 1.0)) {
                throw config::ConfigurationError("Minimum similarity out of range");
            }
        }

        std::unique_ptr<SimilarityMeasure> MakeMeasure(
                util::WorkerThreadPool* thread_pool) const final {
            return std::make_unique<LcsSimilarityMeasure>(min_sim_, thread_pool, size_limit_);
        }
    };

    LcsSimilarityMeasure(model::md::DecisionBoundary min_sim, util::WorkerThreadPool* pool,
                         std::size_t size_limit)
        : ImmediateSimilarityMeasure(
                  std::make_unique<model::StringType>(), true,
                  [min_sim](std::byte const* l, std::byte const* r) {
                      std::string const& left = model::Type::GetValue<model::String>(l);
                      std::string const& right = model::Type::GetValue<model::String>(r);
                      std::size_t dist = LongestCommonSubsequence(left, right);
                      std::size_t const max_dist = std::max(left.size(), right.size());
                      Similarity sim =
                              static_cast<double>(max_dist - dist) / static_cast<double>(max_dist);
                      if (sim < min_sim) return kLowestBound;
                      return sim;
                  },
                  pool, size_limit) {}
};
}  // namespace algos::hymd::preprocessing::similarity_measure
