#pragma once

#include <memory>

#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/selector.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/size_based_selector.h"

namespace algos::hymde::record_match_indexes::rcv_id_selectors {
template <typename T>
class SizeBasedSelectorAdapter final : public Selector<T> {
    std::shared_ptr<SizeBasedSelector const> size_based_selector_;

public:
    SizeBasedSelectorAdapter(std::shared_ptr<SizeBasedSelector const> size_based_selector)
        : size_based_selector_(std::move(size_based_selector)) {}

    std::vector<RecordClassifierValueId> GetSubsetIndices(
            std::vector<T> const& partitioning_values) const final {
        return size_based_selector_->GetSubsetIndices(partitioning_values.size());
    }
};
}  // namespace algos::hymde::record_match_indexes::rcv_id_selectors
