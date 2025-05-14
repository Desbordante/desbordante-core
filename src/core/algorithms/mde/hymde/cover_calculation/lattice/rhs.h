#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/utility/zip.h"
#include "model/index.h"
#include "util/desbordante_assume.h"
#include "util/get_preallocated_vector.h"

namespace algos::hymde::cover_calculation::lattice {
struct Rhs {
    std::unique_ptr<RecordClassifierValueId[]> begin;
    std::size_t non_zero_count;

    Rhs(std::size_t size)
        : begin(std::make_unique<RecordClassifierValueId[]>(size)), non_zero_count(0) {}

    RecordClassifierValueId const& operator[](model::Index index) const noexcept {
        assert(non_zero_count != 0);
        return begin[index];
    }

    bool IsEmpty() const {
        return non_zero_count == 0;
    }

    std::vector<RecordClassifierValueId> DisableAndDo(std::vector<model::Index> const& indices,
                                                      auto action) {
        std::vector<RecordClassifierValueId> rcv_ids =
                util::GetPreallocatedVector<RecordClassifierValueId>(indices.size());
        std::size_t const old_count = non_zero_count;
        // NOTE: not thread-safe
        non_zero_count = 0;
        for (model::Index index : indices) {
            RecordClassifierValueId& removed_rcv_id = begin[index];
            DESBORDANTE_ASSUME(removed_rcv_id != kLowestRCValueId);
            rcv_ids.push_back(removed_rcv_id);
            removed_rcv_id = kLowestRCValueId;
        }
        action(rcv_ids);

        non_zero_count = old_count;
        for (auto [index, rcv_id] : utility::Zip(indices, rcv_ids)) {
            DESBORDANTE_ASSUME(rcv_id != kLowestRCValueId);
            begin[index] = rcv_id;
        }
        return rcv_ids;
    }

    void Set(std::size_t const index, RecordClassifierValueId const value) {
        RecordClassifierValueId& cur_value = begin[index];
        if (cur_value == kLowestRCValueId && value != kLowestRCValueId) {
            ++non_zero_count;
        } else if (cur_value != kLowestRCValueId && value == kLowestRCValueId) {
            --non_zero_count;
        }
        cur_value = value;
    }
};
}  // namespace algos::hymde::cover_calculation::lattice
