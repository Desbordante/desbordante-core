#pragma once

#include <deque>

#include "core/algorithms/fd/bitset_and_index_result_reporter.h"
#include "core/algorithms/fd/lhs_table_mask.h"
#include "core/model/index.h"
#include "core/util/bitset_utils.h"

namespace algos::fd {
// For now, emphasizes the place where FDs are added for easier migration in the future. Will most
// likely be used in some algorithm harnesses as the default result reporting function.
inline BitsetAndIndexResultReporter MakeReorderingLhsMaskAdder(
        std::vector<std::deque<LhsTableMask>>& fds, auto const& ordering) {
    return [&fds, &ordering](boost::dynamic_bitset<> lhs, model::Index rhs_index) {
        fds[ordering[rhs_index]].emplace_back(util::ReorderBitset(lhs, ordering));
    };
}
}  // namespace algos::fd
