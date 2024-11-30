#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"
#include "model/index.h"
#include "util/desbordante_assume.h"
#include "util/get_preallocated_vector.h"

namespace algos::hymd::lattice {
struct Rhs {
    std::unique_ptr<ColumnClassifierValueId[]> begin;
    std::size_t non_zero_count;

    Rhs(std::size_t size)
        : begin(std::make_unique<ColumnClassifierValueId[]>(size)), non_zero_count(0) {}

    ColumnClassifierValueId const& operator[](std::size_t index) const noexcept {
        assert(non_zero_count != 0);
        return begin[index];
    }

    bool IsEmpty() const {
        return non_zero_count == 0;
    }

    std::vector<ColumnClassifierValueId> DisableAndDo(std::vector<model::Index> const& indices,
                                                      auto action) {
        std::vector<ColumnClassifierValueId> ccv_ids =
                util::GetPreallocatedVector<ColumnClassifierValueId>(indices.size());
        std::size_t const old_count = non_zero_count;
        // NOTE: not thread-safe
        non_zero_count = 0;
        for (model::Index index : indices) {
            ColumnClassifierValueId& removed_ccv_id = begin[index];
            DESBORDANTE_ASSUME(removed_ccv_id != kLowestCCValueId);
            ccv_ids.push_back(removed_ccv_id);
            removed_ccv_id = kLowestCCValueId;
        }
        action(ccv_ids);
        non_zero_count = old_count;
        auto ccv_ids_it = ccv_ids.begin();
        for (model::Index index : indices) {
            DESBORDANTE_ASSUME(*ccv_ids_it != kLowestCCValueId);
            begin[index] = *ccv_ids_it++;
        }
        return ccv_ids;
    }

    void Set(std::size_t const index, ColumnClassifierValueId const value) {
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
