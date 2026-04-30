#pragma once

#include <deque>

#include "core/algorithms/fd/bitset_pair_result_reporter.h"
#include "core/algorithms/fd/table_mask_pair.h"
#include "core/util/bitset_utils.h"

namespace algos::fd {
// For now, emphasizes the place where FDs are added for easier migration in the future. Will most
// likely be used in some algorithm harnesses as the default result reporting function.
// For now, we reorder the bits when adding the result.
// Later on, when we open up the guts of the algorithms to the users, the structure that calls the
// result reporting function that will be used in place of this one should not do any reordering.
// The reordering will continue to be performed on the algorithm harness's side.
inline BitsetPairResultReporter MakeReorderingTableMaskPairAdder(std::deque<TableMaskPair>& fds,
                                                                 auto const& ordering) {
    return [&fds, &ordering](boost::dynamic_bitset<> lhs, boost::dynamic_bitset<> rhs) {
        fds.emplace_back(util::ReorderBitset(lhs, ordering), util::ReorderBitset(rhs, ordering));
    };
}
}  // namespace algos::fd
