#pragma once

#include <cstddef>      // for size_t
#include <string>       // for string
#include <string_view>  // for hash

#include "algorithms/od/fastod/hashing/hashing.h"  // for CombineHashes
#include "model/table/column_index.h"              // for ColumnIndex

namespace algos::fastod {

struct AttributePair {
    model::ColumnIndex left;
    model::ColumnIndex right;

    AttributePair();
    AttributePair(model::ColumnIndex left, model::ColumnIndex right);

    std::string ToString() const;

    friend bool operator==(AttributePair const& x, AttributePair const& y);
    friend bool operator!=(AttributePair const& x, AttributePair const& y);
    friend bool operator<(AttributePair const& x, AttributePair const& y);
};

bool operator==(AttributePair const& x, AttributePair const& y);

}  // namespace algos::fastod

namespace std {

template <>
struct hash<algos::fastod::AttributePair> {
    size_t operator()(algos::fastod::AttributePair const& pair) const {
        return algos::fastod::hashing::CombineHashes(pair.left, pair.right);
    }
};

}  // namespace std
