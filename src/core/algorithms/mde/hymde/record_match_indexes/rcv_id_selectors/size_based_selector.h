#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::record_match_indexes::rcv_id_selectors {
class SizeBasedSelector {
public:
    virtual std::vector<RecordClassifierValueId> GetSubsetIndices(std::size_t size) const = 0;

    virtual ~SizeBasedSelector() = default;
};
}  // namespace algos::hymde::record_match_indexes::rcv_id_selectors
