#include "config/indices/validate_index.h"

#include "config/exceptions.h"  // for ConfigurationError
#include "indices/type.h"       // for IndexType

namespace config {

void ValidateIndex(IndexType value, size_t cols_count) {
    if (value >= cols_count) {
        throw ConfigurationError(
                "Column index should be less than the number of columns in the dataset.");
    }
}

}  // namespace config
