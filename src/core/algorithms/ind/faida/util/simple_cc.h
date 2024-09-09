#pragma once

#include <memory>
#include <numeric>
#include <vector>

#include "model/table/column_combination.h"
#include "table/column_index.h"

namespace algos::faida {

using ColumnIndex = model::ColumnIndex;
using TableIndex = model::TableIndex;

class SimpleCC : public model::ColumnCombination {
private:
    int index_;

public:
    SimpleCC(TableIndex table_idx, std::vector<ColumnIndex> col_indices, int index = -1)
        : ColumnCombination(table_idx, std::move(col_indices)), index_(index) {}

    void SetIndex(int index) {
        index_ = index;
    }

    int GetIndex() const {
        return index_;
    }

    ~SimpleCC() override = default;
};

}  // namespace algos::faida

template <>
struct std::hash<algos::faida::SimpleCC> {
    size_t operator()(algos::faida::SimpleCC const& cc) const {
        return cc.GetHash();
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
