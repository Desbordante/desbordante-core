#pragma once

#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/size_based_selector.h"

namespace algos::hymde::record_match_indexes::rcv_id_selectors {
class MaxOnly final : public SizeBasedSelector {
public:
    std::vector<RecordClassifierValueId> GetSubsetIndices(std::size_t size) const final {
        if (size == 1) return {kLowestRCValueId};
        return {kLowestRCValueId, RecordClassifierValueId(size - 1)};
    }
};
}  // namespace algos::hymde::record_match_indexes::rcv_id_selectors
