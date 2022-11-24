#pragma once

#include <memory>
#include <vector>

#include "algorithms/ind/ind.h"

namespace algos::faida {

using ColumnIndex = model::ColumnIndex;
using TableIndex = model::TableIndex;

class SimpleCC : public model::ColumnCombination {
private:
    int index_;

public:
    SimpleCC(TableIndex table_idx, std::vector<ColumnIndex> col_indices)
        : ColumnCombination(table_idx, std::move(col_indices)), index_(-1) {}

    SimpleCC(TableIndex table_idx, std::vector<ColumnIndex> col_indices, int index)
        : ColumnCombination(table_idx, std::move(col_indices)), index_(index) {}

    bool operator==(SimpleCC const& other) const {
        return this->table_index_ == other.table_index_ &&
               this->column_indices_ == other.column_indices_;
    }

    bool operator!=(SimpleCC const& other) const {
        return !(*this == other);
    }

    bool StartsWith(SimpleCC const& other) const {
        return this->table_index_ == other.table_index_ &&
               std::equal(this->column_indices_.begin(), this->column_indices_.end() - 1,
                          other.column_indices_.begin(), other.column_indices_.end() - 1);
    }

    void SetIndex(int index) {
        index_ = index;
    }

    int GetIndex() const {
        return index_;
    }

    int GetLastColumn() const {
        return column_indices_.back();
    }

    ~SimpleCC() override = default;
};

}  // namespace algos::faida

template <>
struct std::hash<algos::faida::SimpleCC> {
    size_t operator()(algos::faida::SimpleCC const& cc) const {
        std::size_t seed = cc.GetColumnIndices().size();
        for (model::ColumnIndex i : cc.GetColumnIndices()) {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

template <>
struct std::hash<std::shared_ptr<algos::faida::SimpleCC>> {
    size_t operator()(std::shared_ptr<algos::faida::SimpleCC> const& cc) const {
        return std::hash<algos::faida::SimpleCC>()(*cc);
    }
};

template <>
struct std::equal_to<std::shared_ptr<algos::faida::SimpleCC>> {
    bool operator()(std::shared_ptr<algos::faida::SimpleCC> const& left,
                    std::shared_ptr<algos::faida::SimpleCC> const& right) const {
        return *left == *right;
    }
};
