#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "table/column_index.h"
#include "table/table_index.h"

namespace algos::cind {
struct Item {
    using ColumnValue = int;
    model::TableIndex table_id;
    model::ColumnIndex column_id;
    ColumnValue value;

    bool operator==(Item const& that) const {
        return this->table_id == that.table_id && this->column_id == that.column_id &&
               this->value == that.value;
    }

    std::string ToString() const noexcept {
        return "{" + std::to_string(table_id) + ", " + std::to_string(column_id) + ", " +
               std::to_string(value) + "}";
    }
};

class Itemset {
public:
    Itemset(std::vector<Item> data) : data_(std::move(data)) {}

    Itemset() = default;

    size_t GetSize() const noexcept {
        return data_.size();
    }

    Item const& GetItem(size_t index) const {
        assert(index < GetSize());
        return data_.at(index);
    }

    Itemset Intersect(Itemset const& that) const {
        if (GetSize() == that.GetSize()) {
            for (size_t index = 0; index < GetSize() - 1; ++index) {
                if (GetItem(index) != that.GetItem(index)) {
                    return {};
                }
            }
            std::vector<Item> data = data_;
            data.emplace_back(that.GetItem(GetSize() - 1));
            return Itemset(std::move(data));
        }
        return {};
    }

private:
    std::vector<Item> data_;
};
}  // namespace algos::cind

template <>
struct std::hash<algos::cind::Item> {
    size_t operator()(algos::cind::Item const& item) const {
        size_t hash = 0;
        hash = (hash << 1) ^ item.table_id;
        hash = (hash << 1) ^ item.column_id;
        hash = (hash << 1) ^ item.value;
        return hash;
    }
};
