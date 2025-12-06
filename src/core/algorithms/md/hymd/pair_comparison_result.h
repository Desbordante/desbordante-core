#pragma once

#include <vector>

#include "core/algorithms/md/hymd/column_classifier_value_id.h"
#include "core/algorithms/md/hymd/lhs_ccv_ids_info.h"
#include "core/algorithms/md/hymd/lowest_cc_value_id.h"
#include "core/algorithms/md/hymd/md_lhs.h"
#include "core/algorithms/md/hymd/utility/zip.h"
#include "core/util/desbordante_assume.h"
#include "core/util/py_tuple_hash.h"

namespace algos::hymd {
// "Similarity set" if using the terminology from "Efficient Discovery of Matching Dependencies".
struct PairComparisonResult {
    std::vector<ColumnClassifierValueId> rhss;
    MdLhs maximal_matching_lhs;

private:
    MdLhs ToLhs(std::vector<LhsCCVIdsInfo> const& lhs_ccv_id_info) const {
        std::size_t offset = 0;
        std::size_t const column_match_number = rhss.size();
        MdLhs lhs{column_match_number};
        DESBORDANTE_ASSUME(column_match_number == lhs_ccv_id_info.size());
        for (auto [rhs_ccv_id, lhs_info] : utility::Zip(rhss, lhs_ccv_id_info)) {
            ColumnClassifierValueId const lhs_ccv_id = lhs_info.rhs_to_lhs_map[rhs_ccv_id];
            if (lhs_ccv_id == kLowestCCValueId) {
                ++offset;
            } else {
                lhs.AddNext(offset) = lhs_ccv_id;
                offset = 0;
            }
        }
        return lhs;
    }

public:
    PairComparisonResult(std::vector<ColumnClassifierValueId> rhss,
                         std::vector<LhsCCVIdsInfo> const& lhs_ccv_id_info)
        : rhss(std::move(rhss)), maximal_matching_lhs(ToLhs(lhs_ccv_id_info)) {}

    friend bool operator==(PairComparisonResult const& l, PairComparisonResult const& r) {
        return l.rhss == r.rhss;
    }
};
}  // namespace algos::hymd

namespace std {
template <>
struct hash<algos::hymd::PairComparisonResult> {
    std::size_t operator()(algos::hymd::PairComparisonResult const& p) const noexcept {
        util::PyTupleHash hasher{p.rhss.size()};
        for (std::size_t el : p.rhss) {
            hasher.AddValue(el);
        }
        return hasher.GetResult();
    }
};
}  // namespace std
