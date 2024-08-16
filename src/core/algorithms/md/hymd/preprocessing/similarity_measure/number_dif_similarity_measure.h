#pragma once

#include "algorithms/md/hymd/preprocessing/similarity_measure/distance_similarity_measure.h"
#include "config/exceptions.h"

namespace algos::hymd::preprocessing::similarity_measure {
inline double NumberDifference(model::Double left, model::Double right) {
    return std::abs(left - right);
}

class NumberSimilarityMeasure : public DistanceSimilarityMeasure<NumberDifference, true> {
    static constexpr auto kName = "number_similarity";

public:
    template <typename... Args>
    NumberSimilarityMeasure(Args&&... args)
        : DistanceSimilarityMeasure<NumberDifference, true>(kName, std::forward<Args>(args)...) {}
};
}  // namespace algos::hymd::preprocessing::similarity_measure
