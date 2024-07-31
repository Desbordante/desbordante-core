#include "algorithms/md/hymd/preprocessing/similarity_measure/equality_metric.h"

template <typename T>
double EqualityMetric(const T& obj1, const T& obj2) {
    return obj1 == obj2 ? 1.0 : 0.0;
}