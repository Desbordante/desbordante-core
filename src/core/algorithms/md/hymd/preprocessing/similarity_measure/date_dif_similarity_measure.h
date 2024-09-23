#pragma once

#include <array>
#include <cstdlib>

#include "algorithms/md/hymd/preprocessing/similarity_measure/distance_similarity_measure.h"
#include "config/exceptions.h"

namespace algos::hymd::preprocessing::similarity_measure {
inline size_t DateDifference(model::Date const& left, model::Date const& right) {
    return std::abs((left - right).days());
}

class DateSimilarityMeasure : public DistanceSimilarityMeasure<DateDifference, true> {
    static constexpr auto kName = "date_similarity";

public:
    template <typename... Args>
    DateSimilarityMeasure(Args&&... args)
        : DistanceSimilarityMeasure<DateDifference, true>(kName, std::forward<Args>(args)...) {}
};

}  // namespace algos::hymd::preprocessing::similarity_measure
