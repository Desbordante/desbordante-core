#pragma once

#include <string>
#include <vector>

#include "column_index.h"

namespace model {

// Represents an index for a table within a set of tables
// As an example, this type is used in inclusion dependencies
using TableIndex = unsigned int;

class ColumnCombination {
protected:
    TableIndex table_index_;
    std::vector<ColumnIndex> column_indices_;

public:
    ColumnCombination(TableIndex table_index, std::vector<ColumnIndex> col_indices)
        : table_index_(table_index), column_indices_(std::move(col_indices)) {}

    TableIndex GetTableIndex() const {
        return table_index_;
    }

    std::vector<ColumnIndex> const& GetColumnIndices() const {
        return column_indices_;
    }

    std::string ToString() const;

    virtual ~ColumnCombination() = default;
};

}  // namespace model
