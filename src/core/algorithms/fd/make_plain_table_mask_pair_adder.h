#pragma once

#include <deque>

#include "core/algorithms/fd/bitset_pair_result_reporter.h"
#include "core/algorithms/fd/table_mask_pair.h"

namespace algos::fd {
// For now, emphasizes the place where FDs are added for easier migration in the future. Will most
// likely be used in some algorithm harnesses as the default result reporting function.
inline BitsetPairResultReporter MakePlainTableMaskPairAdder(std::deque<TableMaskPair>& fds) {
    return [&fds](boost::dynamic_bitset<> lhs, boost::dynamic_bitset<> rhs) {
        fds.emplace_back(std::move(lhs), std::move(rhs));
    };
}
}  // namespace algos::fd
