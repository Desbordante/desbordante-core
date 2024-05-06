#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/jacaard_metric.h"
#include "model/types/string_type.h"

namespace algos::hymd::preprocessing::similarity_measure {

class MongeElkanSimilarityMeasure : public ImmediateSimilarityMeasure {
public:
    MongeElkanSimilarityMeasure(model::md::DecisionBoundary min_sim)
        : ImmediateSimilarityMeasure(std::make_unique<model::StringType>(),
                                     [min_sim](std::byte const* l, std::byte const* r) {
                                         std::string const& left = model::Type::GetValue<model::String>(l);
                                         std::string const& right = model::Type::GetValue<model::String>(r);
                                         Similarity sim = JacaardMetric(left, right);
                                         if (sim < min_sim) return kLowestBound;
                                         return sim;
                                     }) {}
};
}// namespace algos::hymd::preprocessing::similarity_measure