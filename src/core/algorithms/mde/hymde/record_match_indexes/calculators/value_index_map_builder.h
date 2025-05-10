#pragma once

#include <unordered_map>
#include <vector>

#include "algorithms/mde/hymde/partition_value_identifier.h"

namespace algos::hymde::record_match_indexes::calculators {
template <typename ValueType>
class ValueIndexMapBuilder {
    std::vector<ValueType> values_;
    std::unordered_map<ValueType, PartitionValueId> value_partition_id_map_;

public:
    void AddValue(ValueType value, auto&& old_value_action, auto&& new_value_action) {
        auto [it, is_new] = value_partition_id_map_.emplace(std::move(value), values_.size());
        if (is_new) {
            values_.push_back(it->first);
            new_value_action(it->second);
            return;
        }
        old_value_action(it->second);
    }

    std::vector<ValueType> TakeValues() noexcept {
        return std::move(values_);
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
