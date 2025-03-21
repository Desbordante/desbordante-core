#pragma once

#include "algorithms/md/md_verifier/similarities/similarities.h"

namespace algos::md {
class EuclideanSimilarity : public NumericSimilarityMeasure {
public:
    EuclideanSimilarity() : NumericSimilarityMeasure("euclidean") {}

    model::md::Similarity operator()(long double left, long double right) const override;
};

}  // namespace algos::md
