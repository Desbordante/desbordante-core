#pragma once

#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/lcs.h"
#include "config/exceptions.h"
#include "model/types/string_type.h"

namespace algos::hymd::preprocessing::similarity_measure {

class LcsSimilarityMeasure : public NormalImmediateSimilarityMeasure<LongestCommonSubsequence> {
    static constexpr auto kName = "lcs_similarity";

public:
    template <typename... Args>
    LcsSimilarityMeasure(Args&&... args)
        : NormalImmediateSimilarityMeasure<LongestCommonSubsequence>(kName,
                                                                     std::forward<Args>(args)...) {}
};
}  // namespace algos::hymd::preprocessing::similarity_measure
