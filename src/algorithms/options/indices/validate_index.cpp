#include "algorithms/options/indices/validate_index.h"

#include <stdexcept>

namespace algos::config {

void ValidateIndex(IndexType value, size_t cols_count) {
    if (value >= cols_count) {
        throw std::runtime_error(
                "Column index should be less than the number of columns in the dataset.");
    }
}

}  // namespace algos::config
