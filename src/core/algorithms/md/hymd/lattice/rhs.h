#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"

namespace algos::hymd::lattice {
struct Rhs {
    std::unique_ptr<ColumnClassifierValueId[]> begin;
    std::size_t non_zero_count;
    std::size_t const size;

    Rhs(std::size_t size)
        : begin(std::make_unique<ColumnClassifierValueId[]>(size)), non_zero_count(0), size(size) {}

    ColumnClassifierValueId const& operator[](std::size_t index) const noexcept {
        assert(index < size);
        assert(non_zero_count != 0);
        return begin[index];
    }

    void ZeroRhs() {
        non_zero_count = 0;
    }

    bool IsEmpty() const {
        return non_zero_count == 0;
    }

    void Set(std::size_t const index, ColumnClassifierValueId const value) {
        assert(index < size);
        ColumnClassifierValueId& cur_value = begin[index];
        if (cur_value == kLowestCCValueId && value != kLowestCCValueId) {
            ++non_zero_count;
        } else if (cur_value != kLowestCCValueId && value == kLowestCCValueId) {
            --non_zero_count;
        }
        cur_value = value;
    }
};
}  // namespace algos::hymd::lattice
