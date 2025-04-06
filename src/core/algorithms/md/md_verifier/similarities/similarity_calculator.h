#pragma once

#include "algorithms/md/md_verifier/similarities/similarities.h"
#include "model/types/type.h"

namespace algos::md {
class SimilarityCalculator {
private:
    static model::md::Similarity CalculateNumericSimilarity(
            std::byte const* first_val, std::byte const* second_val, model::TypeId type_id,
            std::shared_ptr<NumericSimilarityMeasure> measure);
    static model::md::Similarity CalculateStringSimilarity(
            std::byte const* first_val, std::byte const* second_val, model::TypeId type_id,
            std::shared_ptr<StringSimilarityMeasure> measure);

public:
    static model::md::Similarity Calculate(std::byte const* first_val, std::byte const* second_val,
                                           model::TypeId type_id,
                                           std::shared_ptr<SimilarityMeasure> measure);
};
}  // namespace algos::md
