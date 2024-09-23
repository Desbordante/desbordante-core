#pragma once

#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_metric.h"

namespace algos::hymd::preprocessing::similarity_measure {
class MongeElkanSimilarityMeasure : public NormalImmediateSimilarityMeasure<MongeElkanString> {
    static constexpr auto kName = "monge_elkan_similarity";

public:
    template <typename... Args>
    MongeElkanSimilarityMeasure(Args&&... args)
        : NormalImmediateSimilarityMeasure<MongeElkanString>(kName, std::forward<Args>(args)...) {}
};
}  // namespace algos::hymd::preprocessing::similarity_measure
