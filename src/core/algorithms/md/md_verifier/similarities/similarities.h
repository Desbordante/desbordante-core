#pragma once

#include <vector>

#include <enum.h>

namespace algos::md {
BETTER_ENUM(SimilarityMeasure, char, kEuclidean = 0, kLevenshtein)
using SimilaritiesType = std::vector<SimilarityMeasure>;
}  // namespace algos::md