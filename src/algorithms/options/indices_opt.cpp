#include "algorithms/options/indices_opt.h"

#include <algorithm>
#include <stdexcept>

namespace algos::config {

void TransformIndices(IndicesType& value) {
    if (value.empty()) {
        throw std::invalid_argument("Indices cannot be empty");
    }
    std::sort(value.begin(), value.end());
    value.erase(std::unique(value.begin(), value.end()), value.end());
}

void ValidateIndex(IndexType value, size_t cols_count) {
    if (value >= cols_count) {
        throw std::runtime_error(
                "Column index should be less than the number of columns in the dataset.");
    }
}

void ValidateIndices(IndicesType const& value, size_t cols_count) {
    for (auto index : value) {
        ValidateIndex(index, cols_count);
    }
}

}  // namespace algos::config
