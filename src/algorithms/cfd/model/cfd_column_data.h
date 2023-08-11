#pragma once

#include <utility>
#include <vector>

#include "model/table/abstract_column_data.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

// Column presentation class for CFDRelationData.
class CFDColumnData : model::AbstractColumnData {
    using NumToken = int;

    std::vector<NumToken> values_;

public:
    CFDColumnData(Column const* col, std::vector<int> col_values)
        : AbstractColumnData(col), values_(std::move(col_values)) {}
    explicit CFDColumnData(Column const* col) : AbstractColumnData(col) {}
    std::vector<NumToken> const& GetValues() const {
        return values_;
    }

    std::string ToString() const final {
        return "Cfd data for " + column_->ToString();
    }
};
}  // namespace algos::cfd
