#include "core/algorithms/cfd/cfdfinder/types/columns.h"

namespace algos::cfdfinder {

ColumnRecords::ColumnRecords(RowsPtr&& rows) {
    size_t num_rows = rows->size();
    size_t num_cols = rows->at(0).size();

    columns_.resize(num_cols);
    max_values_.reserve(num_cols);

    for (size_t col = 0; col < num_cols; ++col) {
        columns_[col].reserve(num_rows);
        unsigned int max_val = 0;

        for (size_t row = 0; row < num_rows; ++row) {
            auto val = (*rows)[row][col];
            columns_[col].push_back(val);
            max_val = std::max(max_val, val);
        }
        max_values_.push_back(max_val + 1);
    }
}

}  // namespace algos::cfdfinder