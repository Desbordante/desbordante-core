#include "core/config/column_index/validate_index.h"

#include "core/config/exceptions.h"

namespace config {

void ValidateIndex(IndexType value, size_t cols_count) {
    if (value >= cols_count) {
        throw ConfigurationError(
                "Column index should be less than the number of columns in the dataset.");
    }
}

}  // namespace config
