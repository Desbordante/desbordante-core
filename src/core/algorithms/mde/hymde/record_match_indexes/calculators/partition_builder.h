#pragma once

#include "algorithms/mde/hymde/record_match_indexes/calculators/value_index_map_builder.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"

namespace algos::hymde::record_match_indexes::calculators {
class NullInspector {
public:
    void InspectOld(auto&&...) {}

    void InspectOldLeft(auto&&...) {}

    void InspectOldRight(auto&&...) {}

    void InspectNew(auto&&...) {}

    void InspectNewLeft(auto&&...) {}

    void InspectNewRight(auto&&...) {}
};

template <typename Value, typename AdditionalInspector, typename ValueInfo>
class SinglePBuilder {
    using Adder = PartitionIndex::Adder;

    Adder& left_adder_;
    Adder& right_adder_;
    [[no_unique_address]] AdditionalInspector inspector_;
    ValueIndexMapBuilder<Value> value_map_builder_;

public:
    SinglePBuilder(Adder& left_adder, Adder& right_adder, AdditionalInspector inspector)
        : left_adder_(left_adder), right_adder_(right_adder), inspector_(std::move(inspector)) {}

    void AddValue(Value const& v) {
        auto add_old = [&](PartitionValueId value_id) {
            left_adder_.AddToCluster(value_id);
            right_adder_.AddToCluster(value_id);
            inspector_.InspectOld(v, value_id);
        };
        auto add_new = [&](PartitionValueId value_id) {
            left_adder_.AddToNewCluster(value_id);
            right_adder_.AddToNewCluster(value_id);
            inspector_.InspectNew(v, value_id);
        };

        value_map_builder_.AddValue(v, add_old, add_new);
    }

    ValueInfo Build() {
        return {value_map_builder_.TakeValues(), value_map_builder_.TakeMap(),
                std::move(inspector_)};
    }

    PartitionIndex::PositionListIndex const& GetRightPli() {
        return right_adder_.GetCurrentPli();
    }
};

template <typename LeftElementValue, typename RightElementValue, typename AdditionalInspector,
          typename ValueInfo>
class PairPBuilder {
    using Adder = PartitionIndex::Adder;

    Adder& left_adder_;
    Adder& right_adder_;
    [[no_unique_address]] AdditionalInspector inspector_;
    ValueIndexMapBuilder<LeftElementValue> left_value_map_builder_;
    ValueIndexMapBuilder<RightElementValue> right_value_map_builder_;

public:
    PairPBuilder(Adder& left_adder, Adder& right_adder, AdditionalInspector inspector)
        : left_adder_(left_adder), right_adder_(right_adder), inspector_(std::move(inspector)) {}

    void AddLeftValue(LeftElementValue const& v) {
        auto add_old = [&](PartitionValueId value_id) {
            left_adder_.AddToCluster(value_id);
            inspector_.InspectOldLeft(v, value_id);
        };
        auto add_new = [&](PartitionValueId value_id) {
            left_adder_.AddToNewCluster(value_id);
            inspector_.InspectNewLeft(v, value_id);
        };

        left_value_map_builder_.AddValue(v, add_old, add_new);
    }

    void AddRightValue(RightElementValue const& v) {
        auto add_old = [&](PartitionValueId value_id) {
            right_adder_.AddToCluster(value_id);
            inspector_.InspectOldRight(v, value_id);
        };
        auto add_new = [&](PartitionValueId value_id) {
            right_adder_.AddToNewCluster(value_id);
            inspector_.InspectNewRight(v, value_id);
        };

        right_value_map_builder_.AddValue(v, add_old, add_new);
    }

    ValueInfo Build() {
        return {left_value_map_builder_.TakeValues(), left_value_map_builder_.TakeMap(),
                right_value_map_builder_.TakeValues(), right_value_map_builder_.TakeMap(),
                std::move(inspector_)};
    }

    PartitionIndex::PositionListIndex const& GetRightPli() {
        return right_adder_.GetCurrentPli();
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
