#pragma once

#include <array>
#include <cstdlib>

#include "algorithms/md/hymd/preprocessing/similarity_measure/distance_similarity_measure.h"
#include "algorithms/md/hymd/similarity_measure_creator.h"
#include "config/exceptions.h"
#include "model/types/date_type.h"

namespace algos::hymd::preprocessing::similarity_measure {
inline size_t DateDifference(model::Date const& left, model::Date const& right) {
    return std::abs((left - right).days());
}

class DateSimilarityMeasure : public DistanceSimilarityMeasure {
    static constexpr auto kName = "date_similarity";

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
            return std::make_unique<DateSimilarityMeasure>(min_sim_, thread_pool, size_limit_);
        }
    };

public:
    DateSimilarityMeasure(model::md::DecisionBoundary min_sim, util::WorkerThreadPool* pool,
                          std::size_t size_limit)
        : DistanceSimilarityMeasure(
                  std::make_unique<model::DateType>(), true,
                  [](std::byte const* l, std::byte const* r) {
                      auto const& left = model::Type::GetValue<model::Date>(l);
                      auto const& right = model::Type::GetValue<model::Date>(r);
                      size_t dist = DateDifference(left, right);
                      return dist;
                  },
                  min_sim, pool, size_limit) {}
};

}  // namespace algos::hymd::preprocessing::similarity_measure
