#include "algorithms/md/hymd/preprocessing/similarity_measure/equality_metric.h"

template <typename T>
double EqualityMetric(T const& obj1, T const& obj2) {
    return obj1 == obj2 ? 1.0 : 0.0;
}