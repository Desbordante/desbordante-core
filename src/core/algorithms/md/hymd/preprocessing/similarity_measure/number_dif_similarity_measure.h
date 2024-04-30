#include "algorithms/md/hymd/preprocessing/similarity_measure/distance_similarity_measure.h"

double NumberDifference(const std::byte* left, const std::byte* right) {
    auto left_val = model::Type::GetValue<model::Double>(left);  
    auto right_val = model::Type::GetValue<model::Double>(right);

    return std::abs(static_cast<double>(left_val - right_val));
}

namespace algos::hymd::preprocessing::similarity_measure {
class NumberSimilarityMeasure : public DistanceSimilarityMeasure {
private:
    model::md::DecisionBoundary min_sim;
public:
    NumberSimilarityMeasure(std::unique_ptr<model::Type> arg_type, model::md::DecisionBoundary min_sim)
        : DistanceSimilarityMeasure(std::move(arg_type),
                                     [](std::byte const* l, std::byte const* r) {
                                        double dist = NumberDifference(l, r);
                                        return dist;
                                     }, min_sim) {}
};
}