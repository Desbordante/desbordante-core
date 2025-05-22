#pragma once

#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/basic_partitioner.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/indexes.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/bool.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/selector.h"
#include "util/worker_thread_pool.h"

namespace algos::hymde::record_match_indexes::calculators {
namespace equality {
template <typename T>
class ValueCalculator {
    std::shared_ptr<orders::Bool> order_;

public:
    ComponentHandlingInfo Calculate(ParitioningValuesHolder<T, T>&& values_holder,
                                    PartitionIndex::PositionListIndex const& right_pli,
                                    util::WorkerThreadPool* pool_ptr) const {
        std::vector<RecordClassifierValueId> lhs_ids;
        ClassifierValues classifier_values;
        ValueMatrix value_matrix;
        UpperSetIndex upper_set_index;
        ComponentStructureAssertions structure_assertions;

        std::vector<T> const* left_elements = values_holder.GetLeftPtr();
        std::vector<T> const* right_elements = values_holder.GetRightPtr();
        if (left_elements == right_elements) {
            ;
        } else {
            ;
        }
    }
};

}  // namespace equality

using TTT = std::string;

class Equality : public Calculator {
public:
    using PartitioningFunctionLeft =
            std::unique_ptr<partitioning_functions::PartitioningFunction<TTT>>;
    using PartitioningFunctionRight =
            std::unique_ptr<partitioning_functions::PartitioningFunction<TTT>>;
    using PartitioningFunctionPair = std::pair<PartitioningFunctionLeft, PartitioningFunctionRight>;
    using PartitioningFunctionsOption =
            std::conditional_t<std::is_same_v<TTT, TTT>,
                               std::variant<PartitioningFunctionLeft, PartitioningFunctionPair>,
                               PartitioningFunctionPair>;

private:
    using OrderPtr = std::shared_ptr<orders::Bool const>;

    PartitioningFunctionsOption funcs_;
    records::DictionaryCompressed const& records_;
    OrderPtr order_;
    std::shared_ptr<rcv_id_selectors::Selector<bool> const> selector_;

public:
    [[nodiscard]] ComponentHandlingInfo Calculate(util::WorkerThreadPool* pool_ptr,
                                                  PartitionIndex::Adder&& left_adder,
                                                  PartitionIndex::Adder&& right_adder) const final {
        std::vector<RecordClassifierValueId> lhs_ids;
        ClassifierValues classifier_values;
        ValueMatrix value_matrix;
        UpperSetIndex upper_set_index;
        ComponentStructureAssertions structure_assertions;
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
