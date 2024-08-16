#pragma once

#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_distance.h"
#include "config/exceptions.h"
#include "model/types/string_type.h"

namespace algos::hymd::preprocessing::similarity_measure {

class LevenshteinSimilarityMeasure : public NormalImmediateSimilarityMeasure<LevenshteinDistance> {
    static constexpr auto kName = "levenshtein_similarity";

public:
    template <typename... Args>
    LevenshteinSimilarityMeasure(Args&&... args)
        : NormalImmediateSimilarityMeasure<LevenshteinDistance>(kName,
                                                                std::forward<Args>(args)...) {}
};
}  // namespace algos::hymd::preprocessing::similarity_measure
