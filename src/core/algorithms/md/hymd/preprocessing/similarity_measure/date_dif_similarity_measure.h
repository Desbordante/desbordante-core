#include <array>
#include <cstdlib>

#include "algorithms/md/hymd/preprocessing/similarity_measure/distance_similarity_measure.h"
#include "algorithms/md/hymd/similarity_measure_creator.h"
#include "config/exceptions.h"
#include "model/types/date_type.h"

size_t DateDifference(model::Date const& left, model::Date const& right) {
    return std::abs((left - right).days());
}

namespace algos::hymd::preprocessing::similarity_measure {
class DateSimilarityMeasure : public DistanceSimilarityMeasure {
    static constexpr auto kName = "date_similarity";

public:
    class Creator final : public SimilarityMeasureCreator {
        model::md::DecisionBoundary const min_sim_;

    public:
        Creator(model::md::DecisionBoundary min_sim)
            : SimilarityMeasureCreator(kName), min_sim_(min_sim) {
            if (!(0.0 <= min_sim_ && min_sim_ <= 1.0)) {
                throw config::ConfigurationError("Minimum similarity out of range");
            }
        }

        std::unique_ptr<SimilarityMeasure> MakeMeasure() const final {
            return std::make_unique<DateSimilarityMeasure>(min_sim_);
        }
    };

public:
    DateSimilarityMeasure(model::md::DecisionBoundary min_sim)
        : DistanceSimilarityMeasure(
                  std::make_unique<model::DateType>(),
                  [](std::byte const* l, std::byte const* r) {
                      const auto& left = model::Type::GetValue<model::Date>(l);
                      const auto& right = model::Type::GetValue<model::Date>(r);
                      size_t dist = DateDifference(left, right);
                      return dist;
                  },
                  min_sim) {}
};

}  // namespace algos::hymd::preprocessing::similarity_measure