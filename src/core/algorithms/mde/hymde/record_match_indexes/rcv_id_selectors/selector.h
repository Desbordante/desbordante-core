#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::record_match_indexes::rcv_id_selectors {
template <typename T>
class Selector {
public:
    virtual std::vector<RecordClassifierValueId> GetSubsetIndices(
            std::vector<T> const& partitioning_values) const = 0;

    virtual ~Selector() =  default;
};
}  // namespace algos::hymde::record_match_indexes::rcv_id_selectors
