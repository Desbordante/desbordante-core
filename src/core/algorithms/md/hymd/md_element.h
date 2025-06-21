#pragma once

#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "model/index.h"

namespace algos::hymd {
struct MdElement {
    model::Index index;
    ColumnClassifierValueId ccv_id;
};
}  // namespace algos::hymd
