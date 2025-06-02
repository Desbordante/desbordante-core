#pragma once

#include <unordered_map>
#include <vector>

#include "algorithms/mde/hymde/partition_value_identifier.h"

namespace algos::hymde::record_match_indexes::calculators {
template <typename ValueType, typename Inspector>
struct SinglePartValueInfo {
    [[no_unique_address]] Inspector inspector;
    std::vector<ValueType> values;
    std::unordered_map<ValueType, PartitionValueId> value_partition_id_map;

    SinglePartValueInfo(std::vector<ValueType> values,
                        std::unordered_map<ValueType, PartitionValueId> value_partition_id_map,
                        Inspector inspector)
        : inspector(std::move(inspector)),
          values(std::move(values)),
          value_partition_id_map(std::move(value_partition_id_map)) {}

    std::vector<ValueType> const* GetElements() const noexcept {
        return &values;
    }
};

template <typename LeftValueType, typename RightValueType, typename Inspector>
struct PairPartValueInfo {
    [[no_unique_address]] Inspector inspector;
    std::vector<LeftValueType> left_values;
    std::unordered_map<LeftValueType, PartitionValueId> left_value_partition_id_map;
    std::vector<RightValueType> right_values;
    std::unordered_map<RightValueType, PartitionValueId> right_value_partition_id_map;

    PairPartValueInfo(
            std::vector<LeftValueType> left_values,
            std::unordered_map<LeftValueType, PartitionValueId> left_value_partition_id_map,
            std::vector<RightValueType> right_values,
            std::unordered_map<RightValueType, PartitionValueId> right_value_partition_id_map,
            Inspector inspector)
        : inspector(std::move(inspector)),
          left_values(std::move(left_values)),
          left_value_partition_id_map(std::move(left_value_partition_id_map)),
          right_values(std::move(right_values)),
          right_value_partition_id_map(std::move(right_value_partition_id_map)) {}

    std::vector<LeftValueType> const* GetLeftPtr() const noexcept {
        return &left_values;
    }

    std::vector<RightValueType> const* GetRightPtr() const noexcept {
        return &right_values;
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
