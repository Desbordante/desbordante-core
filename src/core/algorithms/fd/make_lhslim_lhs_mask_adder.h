#pragma once

#include <deque>

#include "core/algorithms/fd/bitset_and_index_result_reporter.h"
#include "core/algorithms/fd/lhs_table_mask.h"
#include "core/config/max_lhs/type.h"
#include "core/model/index.h"

namespace algos::fd {
// For now, emphasizes the place where FDs are added for easier migration in the future. Will most
// likely be used in some algorithm harnesses as the default result reporting function.
inline BitsetAndIndexResultReporter MakeLhsLimLhsMaskAdder(
        std::vector<std::deque<LhsTableMask>>& fds, config::MaxLhsType max_lhs) {
    return [&fds, max_lhs](boost::dynamic_bitset<> lhs, model::Index rhs_index) {
        if (lhs.count() > max_lhs) return;
        fds[rhs_index].emplace_back(std::move(lhs));
    };
}
}  // namespace algos::fd
