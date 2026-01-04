#pragma once

#include <vector>

#include "core/algorithms/md/hymd/column_classifier_value_id.h"

namespace algos::hymd {
struct LhsCCVIdsInfo {
    std::vector<ColumnClassifierValueId> lhs_to_rhs_map;
    std::vector<ColumnClassifierValueId> rhs_to_lhs_map;
};
}  // namespace algos::hymd
