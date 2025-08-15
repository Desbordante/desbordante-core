#pragma once

#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "model/index.h"

namespace algos::hymde::cover_calculation {
struct MdeElement {
    model::Index record_match_index;
    RecordClassifierValueId rcv_id;
};
}  // namespace algos::hymde::cover_calculation
