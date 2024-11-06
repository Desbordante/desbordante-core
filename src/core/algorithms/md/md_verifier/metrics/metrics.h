#pragma once

#include <vector>

#include <enum.h>

namespace algos::md {
BETTER_ENUM(Metric, char, kEuclidean = 0, kLevenshtein)
using MetricsType = std::vector<Metric>;
}  // namespace algos::md