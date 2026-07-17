#pragma once

#include <deque>

#include "core/algorithms/fd/bitset_result_reporter.h"
#include "core/algorithms/fd/lhs_table_mask.h"
#include "core/config/max_lhs/type.h"
#include "core/model/index.h"

namespace algos::fd {
// For now, emphasizes the place where FDs are added for easier migration in the future. Will most
// likely be used in some algorithm harnesses as the default result reporting function.
inline BitsetResultReporter MakePlainLhsMaskAdder(std::deque<boost::dynamic_bitset<>>& fd_lhss) {
    return [&fd_lhss](boost::dynamic_bitset<> lhs) { fd_lhss.push_back(std::move(lhs)); };
}
}  // namespace algos::fd
