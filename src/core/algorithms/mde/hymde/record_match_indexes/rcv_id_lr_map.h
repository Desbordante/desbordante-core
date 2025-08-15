#pragma once

#include <vector>

#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::record_match_indexes {
struct RcvIdLRMap {
    std::vector<RecordClassifierValueId> lhs_to_rhs_map;
    std::vector<RecordClassifierValueId> rhs_to_lhs_map;
};
}  // namespace algos::hymde::record_match_indexes
