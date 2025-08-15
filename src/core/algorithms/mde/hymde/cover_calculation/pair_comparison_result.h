#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_lr_map.h"
#include "algorithms/mde/hymde/utility/zip.h"
#include "util/desbordante_assume.h"
#include "util/py_tuple_hash.h"

namespace algos::hymde::cover_calculation {
struct PairComparisonResult {
    std::vector<RecordClassifierValueId> rhss;
    lattice::PathToNode maximal_matching_lhs;

private:
    lattice::PathToNode ToLhs(
            std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_map) const {
        std::size_t offset = 0;
        std::size_t const record_match_number = rhss.size();
        lattice::PathToNode lhs{record_match_number};
        DESBORDANTE_ASSUME(record_match_number == rcv_id_lr_map.size());
        for (auto [rhs_rcv_id, lhs_info] : utility::Zip(rhss, rcv_id_lr_map)) {
            RecordClassifierValueId const lhs_rcv_id = lhs_info.rhs_to_lhs_map[rhs_rcv_id];
            if (lhs_rcv_id == kLowestRCValueId) {
                ++offset;
            } else {
                lhs.NextStep(offset) = lhs_rcv_id;
                offset = 0;
            }
        }
        return lhs;
    }

public:
    PairComparisonResult(std::vector<RecordClassifierValueId> rhss,
                         std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_map)
        : rhss(std::move(rhss)), maximal_matching_lhs(ToLhs(rcv_id_lr_map)) {}

    // TODO: compare non-intersecting LHSs when added.
    friend bool operator==(PairComparisonResult const& l, PairComparisonResult const& r) {
        return l.rhss == r.rhss;
    }
};
}  // namespace algos::hymde::cover_calculation

namespace std {
template <>
struct hash<algos::hymde::cover_calculation::PairComparisonResult> {
    std::size_t operator()(
            algos::hymde::cover_calculation::PairComparisonResult const& p) const noexcept {
        util::PyTupleHash hasher{p.rhss.size()};
        for (std::size_t el : p.rhss) {
            hasher.AddValue(el);
        }
        return hasher.GetResult();
    }
};
}  // namespace std
