#pragma once

#include <deque>

#include "core/algorithms/fd/bitset_result_reporter.h"
#include "core/algorithms/fd/lhs_table_mask.h"
#include "core/model/index.h"
#include "core/util/bitset_utils.h"

namespace algos::fd {
// For now, emphasizes the place where FDs are added for easier migration in the future. Will most
// likely be used in some algorithm harnesses as the default result reporting function.
inline BitsetResultReporter MakeReorderingLhsMaskAdder(std::deque<LhsTableMask>& fd_lhss,
                                                       auto const& ordering) {
    return [&fd_lhss, &ordering](boost::dynamic_bitset<> lhs) {
        fd_lhss.emplace_back(util::ReorderBitset(lhs, ordering));
    };
}
}  // namespace algos::fd
