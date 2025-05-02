#pragma once

#include <boost/container_hash/hash.hpp>

#include "table/column_index.h"

namespace algos::cind {
struct Item {
    model::ColumnIndex column_id;
    int value;

    bool operator==(Item const& that) const {
        return this->column_id == that.column_id && this->value == that.value;
    }

    bool operator<(Item const& that) const {
        if (this->column_id != that.column_id) {
            return this->column_id < that.column_id;
        }
        return this->value < that.value;
    }
};
}  // namespace algos::cind

template <>
struct std::hash<algos::cind::Item> {
    size_t operator()(algos::cind::Item const& item) const {
        size_t hash = 0;
        boost::hash_combine(hash, boost::hash_value(item.column_id));
        boost::hash_combine(hash, boost::hash_value(item.value));
        return hash;
    }
};
