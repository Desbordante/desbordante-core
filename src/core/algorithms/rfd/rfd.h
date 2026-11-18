#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/config/tabular_data/input_table_type.h"

namespace algos::rfd {

struct RFD {
    std::vector<std::size_t> lhs_indices;
    std::size_t rhs_index = 0;
    double support = 0.0;
    double confidence = 0.0;

    config::InputTable input_table = nullptr;

    auto Key() const {
        return std::tie(lhs_indices, rhs_index);
    }

    auto operator<=>(const RFD& o) const {
        return Key() <=> o.Key();
    }

    bool operator==(const RFD& o) const {
        return Key() == o.Key();
    }

    [[nodiscard]] std::string ToString() const;
};

}  // namespace algos::rfd
