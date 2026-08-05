#pragma once

#include <vector>

#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder {
class ColumnRecords {
private:
    std::vector<std::vector<unsigned int>> columns_;
    std::vector<unsigned int> max_values_;

public:
    explicit ColumnRecords(RowsPtr&& rows);

    std::vector<unsigned int> const& GetColumn(size_t col) const {
        return columns_[col];
    }

    unsigned int GetMaxValue(size_t col) const {
        return max_values_[col];
    }
};
}  // namespace algos::cfdfinder
