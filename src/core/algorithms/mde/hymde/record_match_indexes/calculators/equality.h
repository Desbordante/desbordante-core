#pragma once

#include <memory>

#include "algorithms/mde/decision_boundaries/bool.h"
#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator_impl.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/creator_base.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/partition_builder.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/partition_builder_supplier.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/to_decision_boundaries.h"
#include "algorithms/mde/hymde/record_match_indexes/indexes.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/bool.h"
#include "algorithms/mde/hymde/record_match_indexes/partitioning_functions/partitioning_function.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/selector.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/size_based_selector.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/size_based_selector_adapter.h"
#include "algorithms/mde/hymde/utility/compile_time_value.h"
#include "algorithms/mde/hymde/utility/index_range.h"
#include "config/exceptions.h"
#include "util/worker_thread_pool.h"

namespace algos::hymde::record_match_indexes::calculators {
namespace equality {
template <typename T>
class ValueCalculator {
public:
    using OrderPtr = std::shared_ptr<orders::Bool const>;
    using SelectorPtr = std::shared_ptr<rcv_id_selectors::SizeBasedSelector const>;

private:
    OrderPtr order_;
    SelectorPtr selector_;

public:
    ValueCalculator(OrderPtr order, SelectorPtr selector)
        : order_(std::move(order)), selector_(std::move(selector)) {}

    ComponentHandlingInfo CalculateSingle(auto&& values_holder,
                                          PartitionIndex::PositionListIndex const& right_pli,
                                          util::WorkerThreadPool*) const {
        std::vector<RecordClassifierValueId> lhs_ids;
        ClassifierValues classifier_values;
        ValueMatrix value_matrix;
        UpperSetIndex upper_set_index;
        ComponentStructureAssertions structure_assertions{.assume_record_symmetric_ = true};

        std::vector<T> const& elements = *values_holder.GetElements();
        std::size_t const elements_size = elements.size();
        bool natural_order = order_->GreatestElement() /* == true */;
        if (elements_size == 1) {
            if (natural_order) {
                structure_assertions.assume_overlap_lpli_cluster_max_ = true;
                lhs_ids = {kLowestRCValueId};
                classifier_values = {ToDecisionBoundaries<model::mde::decision_boundaries::Bool>(
                                             std::vector{true}),
                                     false};
            } else {
                structure_assertions.assume_overlap_lpli_cluster_max_ = false;
                // greatest element is false.
                // least element (universal) is true.
                // value compares true. Total is universal
                auto comp_values = std::vector{true, false};
                lhs_ids = selector_->GetSubsetIndices(2);
                classifier_values = {
                        ToDecisionBoundaries<model::mde::decision_boundaries::Bool>(comp_values),
                        true};
            }
        } else {
            if (natural_order) {
                structure_assertions.assume_overlap_lpli_cluster_max_ = true;
                auto comp_values = std::vector{false, true};
                lhs_ids = selector_->GetSubsetIndices(2);
                classifier_values = {
                        ToDecisionBoundaries<model::mde::decision_boundaries::Bool>(comp_values),
                        true};
                for (PartitionValueId value_id : utility::IndexRange(elements_size)) {
                    auto const& cluster = right_pli[value_id];
                    value_matrix.push_back({{value_id, 1}});
                    upper_set_index.push_back(LTPValueUpperSetMapping{
                            LTPVComparisonOrderedRTPValueIDs{value_id},
                            LTPValueRCVIDUpperSetCardinalityMap{{1, {1, cluster.size()}}}});
                }
            } else {
                structure_assertions.assume_overlap_lpli_cluster_max_ = false;
                throw config::ConfigurationError(
                        "Equality with dual order is currently unsupported!");
#if 0
                auto comp_values = std::vector{true, false};
                lhs_ids = selector_->GetSubsetIndices(2);
                classifier_values = {
                        ToDecisionBoundaries<model::mde::decision_boundaries::Bool>(comp_values),
                        true};
                for (PartitionValueId value_id : utility::IndexRange(elements_size)) {
                    auto const& equal_records = right_pli[value_id];
                    OrderedRecords records;
                    EndIdMap end_ids;

                    ValueMatrixRow& row = value_matrix.emplace_back(elements_size - 1);
                    for (PartitionValueId value_id : utility::IndexRange(value_id)) {
                        ;
                    }
                    for (PartitionValueId value_id :
                         std::views::iota(value_id + 1, elements_size)) {
                        ;
                    }
                    value_matrix.push_back({{value_id, 1}});
                    // upper_set_index.push_back({{cluster, {{1, cluster.size()}}}});
                }
#endif
            }
        }

        return {{std::move(classifier_values), std::move(lhs_ids)},
                {std::move(value_matrix), std::move(upper_set_index)},
                structure_assertions};
    }

    ComponentHandlingInfo CalculatePair(auto&&, PartitionIndex::PositionListIndex const&,
                                        util::WorkerThreadPool*) const {
        throw config::ConfigurationError("Equality on two tables is currently unsupported!");
    }

    std::string GetOrderName() const {
        return order_->ToString();
    }
};

template <typename PartitioningValue>
using BuilderSupplier =
        SameValueTypeBuilderSupplier<PartitioningValue, NullInspector, NullInspector>;

using EqValue = utility::CompileTimeOptionalLike<true>;
using Symmetric = utility::CompileTimeValue<true>;

template <typename PartitioningValue>
using Base = CalculatorImpl<BuilderSupplier<PartitioningValue>, ValueCalculator<PartitioningValue>>;
}  // namespace equality

template <typename T>
class Equality : public equality::Base<T> {
public:
    using PartitioningValueLeft = T;
    using PartitioningValueRight = T;

    using PartitioningFunctionLeft =
            std::unique_ptr<partitioning_functions::PartitioningFunction<T>>;
    using PartitioningFunctionRight =
            std::unique_ptr<partitioning_functions::PartitioningFunction<T>>;
    using PartitioningFunctionPair = std::pair<PartitioningFunctionLeft, PartitioningFunctionRight>;
    using PartitioningFunctionsOption =
            std::variant<PartitioningFunctionLeft, PartitioningFunctionPair>;

    using OrderPtr = std::shared_ptr<orders::Bool const>;
    using SelectorPtr = equality::ValueCalculator<T>::SelectorPtr;

public:
    class Creator : public CreatorBase<Equality> {
        using Base = CreatorBase<Equality>;

    public:
        using PartitioningFunctionCreatorsOption = Base::PartitioningFunctionCreatorsOption;
        using ComparisonResult = bool;
        using OrderPtr = Equality<T>::OrderPtr;
        // TODO: predicate creator.
        using SelectorPtr = Equality<T>::SelectorPtr;

    private:
        OrderPtr order_ptr_;
        SelectorPtr selector_ptr_;

    public:
        Creator(PartitioningFunctionCreatorsOption pf_creators, OrderPtr order_ptr,
                SelectorPtr selector_ptr)
            : Base(std::move(pf_creators)),
              order_ptr_(std::move(order_ptr)),
              selector_ptr_(std::move(selector_ptr)) {}

        std::unique_ptr<Calculator> Create(
                RelationalSchema const& left_schema, RelationalSchema const& right_schema,
                records::DictionaryCompressed const& records) const final {
            return std::make_unique<Equality>(
                    records, Base::MakePartFuncs(left_schema, right_schema, records), order_ptr_,
                    selector_ptr_);
        }
    };

    Equality(records::DictionaryCompressed const& records,
             PartitioningFunctionsOption partitioning_functions, OrderPtr order,
             SelectorPtr selector)
        : equality::Base<T>("equality", std::move(partitioning_functions), records, {},
                            {std::move(order), std::move(selector)}) {}
};
}  // namespace algos::hymde::record_match_indexes::calculators
