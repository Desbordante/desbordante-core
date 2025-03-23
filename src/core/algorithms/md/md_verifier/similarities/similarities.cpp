#include "algorithms/md/md_verifier/similarities/similarities.h"

#include <stdexcept>

namespace algos::md {

std::shared_ptr<NumericSimilarityMeasure> AsNumericMeasure(
        std::shared_ptr<SimilarityMeasure> const& ptr) {
    std::shared_ptr<NumericSimilarityMeasure> casted_ptr =
            std::dynamic_pointer_cast<NumericSimilarityMeasure>(ptr);
    if (!casted_ptr.get()) {
        throw std::runtime_error("Failed to cast" + ptr->GetName() +
                                 " similarity measure to numeric similarity measure");
    }
    return casted_ptr;
}

std::shared_ptr<StringSimilarityMeasure> AsStringMeasure(
        std::shared_ptr<SimilarityMeasure> const& ptr) {
    std::shared_ptr<StringSimilarityMeasure> casted_ptr =
            std::dynamic_pointer_cast<StringSimilarityMeasure>(ptr);
    if (!casted_ptr.get()) {
        throw std::runtime_error("Failed to cast" + ptr->GetName() +
                                 " similarity measure to string similarity measure");
    }
    return casted_ptr;
}
}  // namespace algos::md
