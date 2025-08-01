#pragma once

#include <type_traits>

#include "algorithms/mde/hymde/record_match_indexes/calculators/part_value_info.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/partition_builder.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"

namespace algos::hymde::record_match_indexes::calculators {
template <typename LeftValue, typename RightValue, typename PairInspector>
class PartitionBuilderBase {
    static_assert(std::is_default_constructible_v<PairInspector>,
                  "Value inspector must be default constructible for now.");

    using Adder = PartitionIndex::PartitionBuilder;
    using PairInfo = PairPartValueInfo<LeftValue, RightValue, PairInspector>;
    using PairBuilder = PairPBuilder<LeftValue, RightValue, PairInspector, PairInfo>;

public:
    using LeftElement = LeftValue;
    using RightElement = RightValue;

    PartitionBuilderBase() = default;

    PairBuilder GetPair(Adder& left, Adder& right) const {
        return {left, right, {}};
    }
};

template <typename Value, typename SingleInspector, typename PairInspector>
class SameValueTypeBuilderSupplier : public PartitionBuilderBase<Value, Value, PairInspector> {
    static_assert(std::is_default_constructible_v<SingleInspector>,
                  "Value inspector must be default constructible for now.");

    using SingleInfo = SinglePartValueInfo<Value, SingleInspector>;
    using SingleBuilder = SinglePBuilder<Value, SingleInspector, SingleInfo>;
    using Adder = PartitionIndex::PartitionBuilder;

public:
    SingleBuilder GetSingle(Adder& left, Adder& right) const {
        return {left, right, {}};
    }
};

template <typename LeftValue, typename RightValue, typename PairInspector>
class PairOnlyBuilderSupplier : public PartitionBuilderBase<LeftValue, RightValue, PairInspector> {
};
}  // namespace algos::hymde::record_match_indexes::calculators
