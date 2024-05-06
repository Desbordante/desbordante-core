#include <array>
#include <cstdlib>

#include "algorithms/md/hymd/preprocessing/similarity_measure/distance_similarity_measure.h"
#include "model/types/date_type.h"

size_t DateDifference(model::Date const& left, model::Date const& right) {
    return std::abs((left - right).days());
}

namespace algos::hymd::preprocessing::similarity_measure {
class DateSimilarityMeasure : public DistanceSimilarityMeasure {
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