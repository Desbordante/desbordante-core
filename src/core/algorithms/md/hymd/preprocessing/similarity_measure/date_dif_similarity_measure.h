#include "algorithms/md/hymd/preprocessing/similarity_measure/distance_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/date_difference.h"

#include <array>
#include <cstdlib>

namespace algos::hymd::preprocessing::similarity_measure {
class DateSimilarityMeasure : public DistanceSimilarityMeasure {
public:
    DateSimilarityMeasure(model::md::DecisionBoundary min_sim)
        : DistanceSimilarityMeasure(std::make_unique<model::DateType>(),
                                     [](std::byte const* l, std::byte const* r) {
                                        const auto& left = model::Type::GetValue<model::Date>(l);
                                        const auto& right = model::Type::GetValue<model::Date>(r);
                                        size_t dist = DateDifference(left, right);
                                        return dist;
                                     }, min_sim) {}
};
}