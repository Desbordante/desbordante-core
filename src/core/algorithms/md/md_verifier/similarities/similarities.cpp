#include "algorithms/md/md_verifier/similarities/similarities.h"

#include <stdexcept>

namespace algos::md {

std::shared_ptr<NumericSimilarityMeasure> AsNumericMeasure(
        std::shared_ptr<SimilarityMeasure>& ptr) {
    auto ptr_ = std::dynamic_pointer_cast<NumericSimilarityMeasure>(ptr);
    if (!ptr_.get()) {
        throw std::runtime_error("Failed to cast" + ptr->GetName() +
                                 "similarity measure to numeric similarity measure");
    }
    return ptr_;
}

std::shared_ptr<StringSimilarityMeasure> AsStringMeasure(std::shared_ptr<SimilarityMeasure>& ptr) {
    auto ptr_ = std::dynamic_pointer_cast<StringSimilarityMeasure>(ptr);
    if (!ptr_.get()) {
        throw std::runtime_error("Failed to cast" + ptr->GetName() +
                                 "similarity measure to string similarity measure");
    }
    return ptr_;
}
}  // namespace algos::md
