#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace algos::rfd {

struct RFD {
    std::vector<std::string> lhs;
    std::string rhs;
    std::vector<std::size_t> lhs_indices;
    std::size_t rhs_index = 0;
    double support = 0.0;
    double confidence = 0.0;

    bool operator==(const RFD& other) const {
        // support/confidence are derived values and may differ
        return lhs_indices == other.lhs_indices && rhs_index == other.rhs_index;
    }

    auto operator<=>(const RFD& other) const {
        if (auto cmp = lhs_indices <=> other.lhs_indices; cmp != 0) return cmp;
        return rhs_index <=> other.rhs_index;
    }

    [[nodiscard]] std::string ToString() const;
};

}  // namespace algos::rfd
