#pragma once

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "arity_index.h"
#include "column_index.h"
#include "table_index.h"

namespace model {

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

    ColumnIndex GetLastColumn() const noexcept {
        return column_indices_.back();
    }

    ArityIndex GetArity() const noexcept {
        return static_cast<ArityIndex>(GetColumnIndices().size());
    }

    ColumnCombination WithColumn(ColumnIndex col_index) const {
        ColumnCombination cc{*this};
        cc.column_indices_.push_back(col_index);
        return cc;
    }

    ColumnCombination WithoutIndex(ArityIndex index_id) const {
        ColumnCombination cc{*this};
        cc.column_indices_.erase(cc.column_indices_.begin() + index_id);
        return cc;
    }

    bool operator==(ColumnCombination const& other) const {
        return this->table_index_ == other.table_index_ &&
               this->column_indices_ == other.column_indices_;
    }

    bool operator!=(ColumnCombination const& other) const {
        return !(*this == other);
    }

    size_t GetHash() const {
        std::vector<model::ColumnIndex> const& indices = GetColumnIndices();
        return std::accumulate(indices.begin(), indices.end(), indices.size(),
                               [](std::size_t seed, model::ColumnIndex i) {
                                   return seed ^ (i + 0x9e3779b9 + (seed << 6) + (seed >> 2));
                               });
    }

    bool StartsWith(ColumnCombination const& other) const {
        return this->table_index_ == other.table_index_ &&
               std::equal(this->column_indices_.begin(), this->column_indices_.end() - 1,
                          other.column_indices_.begin(), other.column_indices_.end() - 1);
    }

    static bool HaveIndicesIntersection(ColumnCombination const& lhs,
                                        ColumnCombination const& rhs) noexcept;

    std::string ToString() const;

    virtual ~ColumnCombination() = default;
};

}  // namespace model
