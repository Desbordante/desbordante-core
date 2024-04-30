#include "algorithms/md/hymd/preprocessing/similarity_measure/distance_similarity_measure.h"
#include "model/types/date_type.h"

size_t DateDifference(const std::byte* left, const std::byte* right) {
    auto left_date = model::Type::GetValue<model::Date>(left);
    auto right_date = model::Type::GetValue<model::Date>(right);

    model::DateType date_type;
    return abs(date_type.SubDate(left, right).days());
}

namespace algos::hymd::preprocessing::similarity_measure {
class DateSimilarityMeasure : public DistanceSimilarityMeasure {
public:
    DateSimilarityMeasure(std::unique_ptr<model::Type> arg_type, model::md::DecisionBoundary min_sim)
        : DistanceSimilarityMeasure(std::move(arg_type),
                                     [](std::byte const* l, std::byte const* r) {
                                        size_t dist = DateDifference(l, r);
                                        return dist;
                                     }, min_sim) {}
};
}