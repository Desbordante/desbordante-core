#pragma once

#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/jaccard_metric.h"

namespace algos::hymd::preprocessing::similarity_measure {

class JaccardSimilarityMeasure : public NormalImmediateSimilarityMeasure<StringJaccardIndex> {
    static constexpr auto kName = "jaccard_similarity";

public:
    template <typename... Args>
    JaccardSimilarityMeasure(Args&&... args)
        : NormalImmediateSimilarityMeasure<StringJaccardIndex>(kName, std::forward<Args>(args)...) {
    }
};
}  // namespace algos::hymd::preprocessing::similarity_measure
