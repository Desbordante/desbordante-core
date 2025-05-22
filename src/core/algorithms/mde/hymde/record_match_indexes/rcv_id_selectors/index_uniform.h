#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/size_based_selector.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::record_match_indexes::rcv_id_selectors {
class IndexUniform final : public SizeBasedSelector {
    std::size_t size_limit_;

public:
    std::vector<RecordClassifierValueId> GetSubsetIndices(std::size_t size) const final;

    IndexUniform(std::size_t size_limit = 0) : size_limit_(size_limit) {}
};
}  // namespace algos::hymde::record_match_indexes::rcv_id_pickers
