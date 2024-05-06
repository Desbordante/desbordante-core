#include "algorithms/md/hymd/preprocessing/similarity_measure/distance_similarity_measure.h"

double NumberDifference(model::Double left, model::Double right) {
    return std::abs(left - right);
}

namespace algos::hymd::preprocessing::similarity_measure {
class NumberSimilarityMeasure : public DistanceSimilarityMeasure {
public:
    NumberSimilarityMeasure(model::md::DecisionBoundary min_sim)
        : DistanceSimilarityMeasure(
                  std::make_unique<model::DoubleType>(),
                  [](std::byte const* l, std::byte const* r) {
                      model::Double left_val = model::Type::GetValue<model::Double>(l);
                      model::Double right_val = model::Type::GetValue<model::Double>(r);
                      return NumberDifference(left_val, right_val);
                  },
                  min_sim) {}
};
}  // namespace algos::hymd::preprocessing::similarity_measure