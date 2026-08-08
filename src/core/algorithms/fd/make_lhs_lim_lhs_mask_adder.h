#pragma once

#include <deque>

#include "core/algorithms/fd/bitset_result_reporter.h"
#include "core/algorithms/fd/lhs_table_mask.h"
#include "core/config/max_lhs/type.h"

namespace algos::fd {
// For now, emphasizes the place where FDs are added for easier migration in the future. Will most
// likely be used in some algorithm harnesses as the default result reporting function.
inline BitsetResultReporter MakeLhsLimLhsMaskAdder(std::deque<LhsTableMask>& fd_lhss,
                                                   config::MaxLhsType max_lhs) {
    return [&fd_lhss, max_lhs](LhsTableMask lhs) {
        if (lhs.count() > max_lhs) return;
        fd_lhss.push_back(std::move(lhs));
    };
}
}  // namespace algos::fd
