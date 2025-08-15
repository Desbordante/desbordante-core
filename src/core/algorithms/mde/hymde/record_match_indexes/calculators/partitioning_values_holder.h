#pragma once

#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "algorithms/mde/hymde/partition_value_identifier.h"

namespace algos::hymde::record_match_indexes::calculators {
template <typename L, typename R>
class ParitioningValuesHolder {
    using LeftValues = std::vector<L>;
    using RightValues = std::vector<R>;
    using ValuePair = std::pair<LeftValues, RightValues>;

    std::variant<LeftValues, ValuePair> values_;
    LeftValues const* left_ptr_;
    RightValues const* right_ptr_;

public:
    ParitioningValuesHolder(LeftValues left_values)
        : values_(std::move(left_values)),
          left_ptr_(std::get_if<LeftValues>(&values_)),
          right_ptr_(left_ptr_) {}

    ParitioningValuesHolder(LeftValues left_values, RightValues right_values)
        : values_(std::in_place_type<ValuePair>, std::move(left_values), std::move(right_values)),
          left_ptr_(&std::get<ValuePair>(values_).first),
          right_ptr_(&std::get<ValuePair>(values_).second) {}

    LeftValues const* GetLeftPtr() const noexcept {
        return left_ptr_;
    }

    RightValues const* GetRightPtr() const noexcept {
        return right_ptr_;
    }

    ParitioningValuesHolder(ParitioningValuesHolder const&) = delete;
    ParitioningValuesHolder(ParitioningValuesHolder&&) = delete;
    ParitioningValuesHolder& operator=(ParitioningValuesHolder const&) = delete;
    ParitioningValuesHolder& operator=(ParitioningValuesHolder&&) = delete;
};
}  // namespace algos::hymde::record_match_indexes::calculators
