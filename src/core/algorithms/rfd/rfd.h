#pragma once

#include <cstdint>
#include <string>

namespace algos::rfd {

struct RFD {
    uint32_t lhs_mask = 0;
    uint8_t rhs_index = 0;
    double support = 0.0;
    double confidence = 0.0;

    inline bool operator<(const RFD& other) const {
        if (lhs_mask != other.lhs_mask) return lhs_mask < other.lhs_mask;
        return rhs_index < other.rhs_index;
    }

    [[nodiscard]] std::string ToString() const;
};

}  // namespace algos::rfd