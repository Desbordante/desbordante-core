#pragma once

#include <cstddef>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include "core/model/table/encoded_column_data.h"

namespace algos::cind::utils {

struct VecIntHash {
    std::size_t operator()(std::vector<int> const& vec) const noexcept {
        return boost::hash_value(vec);
    }
};

inline std::vector<int> MakeKey(std::size_t row,
                                std::vector<model::EncodedColumnData const*> const& cols) {
    std::vector<int> key;
    key.reserve(cols.size());
    for (auto const* c : cols) {
        key.push_back(c->GetValue(row));
    }
    return key;
}

}  // namespace algos::cind::utils
