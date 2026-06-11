#pragma once

#include <cstdint>
#include <string>

namespace algos::rfd {

// Masks use uint32_t -> max 31 attributes (31 bits + RHS can't overlap)
static constexpr std::size_t kMaxAttributes = 31;

struct RFD {
    uint32_t lhs_mask = 0;
    uint8_t rhs_index = 0;
    double support = 0.0;
    double confidence = 0.0;

    bool operator==(const RFD& other) const {
        // Identity is defined only by (lhs_mask, rhs_index);
        // conf/supp are derived values and may differ
        return lhs_mask == other.lhs_mask && rhs_index == other.rhs_index;
    }

    auto operator<=>(const RFD& other) const {
        if (auto cmp = lhs_mask <=> other.lhs_mask; cmp != 0) return cmp;
        return rhs_index <=> other.rhs_index;
    }

    [[nodiscard]] std::string ToString() const;
};

struct RFDHash {
    std::size_t operator()(const RFD& rfd) const {
        return (static_cast<std::size_t>(rfd.lhs_mask) << 8) | rfd.rhs_index;
    }
};

}  // namespace algos::rfd
