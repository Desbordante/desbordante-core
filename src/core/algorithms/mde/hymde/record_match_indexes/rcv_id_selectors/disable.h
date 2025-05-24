#pragma once

#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/size_based_selector.h"

namespace algos::hymde::record_match_indexes::rcv_id_selectors {
class Disable final : public SizeBasedSelector {
public:
    std::vector<RecordClassifierValueId> GetSubsetIndices(std::size_t) const final {
        return {kLowestRCValueId};
    }
};
}  // namespace algos::hymde::record_match_indexes::rcv_id_selectors
