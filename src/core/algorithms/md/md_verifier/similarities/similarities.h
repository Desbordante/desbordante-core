#pragma once

#include <vector>

#include <enum.h>

namespace algos::md {
BETTER_ENUM(SimilarityMeasure, char, kEuclidean = 0, kLevenshtein)
using SimilaritiesType = std::vector<SimilarityMeasure>;

template <typename T>
class AbstractSimilarityMeasure {
public:
    virtual long double operator()(T left, T right) = 0;
};
}  // namespace algos::md
