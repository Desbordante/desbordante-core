#pragma once

#include <memory>

#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/build_indexes.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/encode_results.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/meaningful_table_results.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/partitioning_values_holder.h"
#include "algorithms/mde/hymde/record_match_indexes/indexes.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/selector.h"
#include "algorithms/mde/hymde/utility/index_range.h"
#include "util/argument_type.h"
#include "util/worker_thread_pool.h"

namespace algos::hymde::record_match_indexes::calculators {
template <typename ComparerCreatorSupplier, typename DecisionBoundaryType, bool Symmetric,
          bool EqMax, bool EnableMultithreading = true>
class BasicValueCalculatorCalc {
    using LeftElementsArg = util::ArgumentType<ComparerCreatorSupplier, 0>;
    using RightElementsArg = util::ArgumentType<ComparerCreatorSupplier, 1>;

    using LeftElements = std::remove_cvref_t<std::remove_pointer_t<LeftElementsArg>>;
    using RightElements = std::remove_cvref_t<std::remove_pointer_t<RightElementsArg>>;

    using LeftElementType = LeftElements::value_type;
    using RightElementType = RightElements::value_type;
    // Acquires resources for Comparer based on the data.
    using ComparerCreator =
            std::invoke_result_t<ComparerCreatorSupplier, LeftElementsArg, RightElementsArg>;
    // Does the actual comparisons between values, may store resources acquired prior, like a memory
    // buffer
    using Comparer = std::invoke_result_t<ComparerCreator>;
    using ComparisonResult = std::invoke_result_t<Comparer, LeftElementType, RightElementType>;

    struct ThreadResource {
        Comparer comparer;
        bool least_element_found = false;
    };

    class Worker {
        using LeftValueComparisonInfoOrigType = LeftValueComparisonInfo<ComparisonResult>;
        std::vector<LeftElementType> const& left_elements_;
        std::vector<RightElementType> const& right_elements_;
        PartitionIndex::PositionListIndex const& right_pli_;
        orders::TotalOrder<ComparisonResult> const& order_;
        ComparerCreator create_comparer_;
        std::size_t const num_values_left_ = left_elements_.size();
        std::size_t const num_values_right_ = right_elements_.size();
        MeaningfulDataResults<ComparisonResult> task_data_ =
                util::GetPreallocatedVector<LeftValueComparisonInfoOrigType>(num_values_left_);
        ComparisonResult least_element_ = order_.LeastElement();
        ComparisonResult greatest_element_ = order_.GreatestElement();

        void AddRightPartValIdCompResPair(LeftValueComparisonInfoOrigType& lv_comp_info,
                                          PartitionValueId pvalue_id_right,
                                          ComparisonResult comp_res) {
            auto& [comp_part_value_id_pairs, meaningful_records_number] = lv_comp_info;
            comp_part_value_id_pairs.emplace_back(std::move(comp_res), pvalue_id_right);
            meaningful_records_number += right_pli_[pvalue_id_right].size();
        }

        void CalcOnePair(Comparer& comparer, LeftValueComparisonInfoOrigType& lv_comp_info,
                         LeftElementType const& left_element, PartitionValueId pvalue_id_right,
                         bool& least_element_found) {
            RightElementType const& right_element = right_elements_[pvalue_id_right];
            ComparisonResult comp_res = comparer(left_element, right_element);
            if (comp_res == least_element_) {
                least_element_found = true;
                return;
            }
            AddRightPartValIdCompResPair(lv_comp_info, pvalue_id_right, std::move(comp_res));
        }

        void CalcLoop(Comparer& comparer, LeftValueComparisonInfoOrigType& lv_comp_info,
                      LeftElementType const& left_element, bool& least_element_found,
                      PartitionValueId from, PartitionValueId to) {
            for (PartitionValueId pvalue_id_right : std::views::iota(from, to)) {
                CalcOnePair(comparer, lv_comp_info, left_element, pvalue_id_right,
                            least_element_found);
            }
        }

        void CalcForFull(Comparer& comparer, PartitionValueId pvalue_id_left,
                         bool& least_element_found) {
            LeftElementType const& left_element = left_elements_[pvalue_id_left];
            LeftValueComparisonInfoOrigType& lv_comp_info = task_data_[pvalue_id_left];
            CalcLoop(comparer, lv_comp_info, left_element, least_element_found, 0,
                     num_values_right_);
        }

        void CalcForSame(Comparer& comparer, PartitionValueId pvalue_id_left,
                         bool& least_element_found) {
            LeftElementType const& left_element = left_elements_[pvalue_id_left];
            LeftValueComparisonInfoOrigType& lv_comp_info = task_data_[pvalue_id_left];
            if constexpr (!Symmetric) {
                CalcLoop(comparer, lv_comp_info, left_element, least_element_found, 0,
                         pvalue_id_left);
            }
            if constexpr (EqMax) {
                AddRightPartValIdCompResPair(lv_comp_info, pvalue_id_left, greatest_element_);
            } else {
                CalcOnePair(comparer, lv_comp_info, left_element, pvalue_id_left,
                            least_element_found);
            }
            CalcLoop(comparer, lv_comp_info, left_element, least_element_found, pvalue_id_left + 1,
                     num_values_right_);
        }

        auto Enumerate(bool least_element_found) {
            return EncodeResults(std::move(task_data_), &order_, least_element_found);
        }

        auto GetCalculationMethod() const noexcept {
            return OneFunctionForOneTableGiven() ? &Worker::CalcForSame : &Worker::CalcForFull;
        }

        ThreadResource AcquireResource() {
            return {create_comparer_()};
        }

    public:
        Worker(std::vector<LeftElementType> const* left_elements,
               std::vector<RightElementType> const* right_elements,
               PartitionIndex::PositionListIndex const& right_clusters,
               orders::TotalOrder<ComparisonResult> const& order, ComparerCreator create_comparer)
            : left_elements_(*left_elements),
              right_elements_(*right_elements),
              right_pli_(right_clusters),
              order_(order),
              create_comparer_(std::move(create_comparer)) {}

        bool OneFunctionForOneTableGiven() const noexcept {
            return &left_elements_ == &right_elements_;
        }

        auto ExecSingleThreaded() {
            auto calculation_method = GetCalculationMethod();
            auto [comparer, least_element_found] = AcquireResource();
            for (PartitionValueId left_pvalue_id : utility::IndexRange(num_values_left_)) {
                task_data_.emplace_back();
                (this->*calculation_method)(comparer, left_pvalue_id, least_element_found);
            }
            return Enumerate(least_element_found);
        }

        auto ExecMultiThreaded(util::WorkerThreadPool& pool) {
            task_data_.assign(num_values_left_, {});
            std::atomic<bool> least_element_found = false;
            auto calculation_method = GetCalculationMethod();
            auto set_least_element = [&least_element_found](ThreadResource resource) {
                if (resource.least_element_found)
                    least_element_found.store(true, std::memory_order::release);
            };
            auto calculate = [this, calculation_method](PartitionValueId left_pvalue_id,
                                                        ThreadResource& resource) {
                (this->*calculation_method)(resource.comparer, left_pvalue_id,
                                            resource.least_element_found);
            };
            auto acquire_resource = [this]() { return AcquireResource(); };
            pool.ExecIndexWithResource(calculate, acquire_resource, num_values_left_,
                                       set_least_element);
            return Enumerate(least_element_found.load(std::memory_order::acquire));
        }
    };

    // May store the comparison function.
    ComparerCreatorSupplier const* creator_supplier_;
    orders::TotalOrder<ComparisonResult> const* order_;
    rcv_id_selectors::Selector<ComparisonResult> const* selector_;

public:
    BasicValueCalculatorCalc(ComparerCreatorSupplier const* creator_supplier,
                             orders::TotalOrder<ComparisonResult> const* order,
                             rcv_id_selectors::Selector<ComparisonResult> const* selector)
        : creator_supplier_(creator_supplier), order_(order), selector_(selector) {}

    ComponentHandlingInfo Calculate(
            ParitioningValuesHolder<LeftElementType, RightElementType>&& values_holder,
            PartitionIndex::PositionListIndex const& right_pli,
            util::WorkerThreadPool* pool_ptr) const {
        LeftElements const* left_elements = values_holder.GetLeftPtr();
        RightElements const* right_elements = values_holder.GetRightPtr();
        ComparerCreator create_comparer = (*creator_supplier_)(left_elements, right_elements);
        Worker worker{left_elements, right_elements, right_pli, *order_,
                      std::move(create_comparer)};
        auto [classifier_values, enumerated_results, least_element_found] =
                EnableMultithreading && pool_ptr != nullptr ? worker.ExecMultiThreaded(*pool_ptr)
                                                            : worker.ExecSingleThreaded();
        if constexpr (Symmetric) {
            if (worker.OneFunctionForOneTableGiven())
                SymmetricClosure(enumerated_results, right_pli);
        }
        ComponentStructureAssertions assertions{
                .assume_overlap_lpli_cluster_max_ = EqMax && worker.OneFunctionForOneTableGiven(),
                .assume_record_symmetric_ = Symmetric && worker.OneFunctionForOneTableGiven()};
        return BuildIndexes<DecisionBoundaryType>(std::move(enumerated_results),
                                                  std::move(classifier_values), right_pli,
                                                  *selector_, least_element_found, assertions);
    }
};

template <typename ComparerCreatorSupplier, typename DecisionBoundaryType, bool Symmetric,
          bool EnableMultithreading = true>
class BasicValueCalculator {
    using LeftElementsArg = util::ArgumentType<ComparerCreatorSupplier, 0>;
    using RightElementsArg = util::ArgumentType<ComparerCreatorSupplier, 1>;

    using LeftElements = std::remove_cvref_t<std::remove_pointer_t<LeftElementsArg>>;
    using RightElements = std::remove_cvref_t<std::remove_pointer_t<RightElementsArg>>;

    using LeftElementType = LeftElements::value_type;
    using RightElementType = RightElements::value_type;
    // Acquires resources for Comparer based on the data.
    using ComparerCreator =
            std::invoke_result_t<ComparerCreatorSupplier, LeftElementsArg, RightElementsArg>;
    // Does the actual comparisons between values, may store resources acquired prior, like a memory
    // buffer
    using Comparer = std::invoke_result_t<ComparerCreator>;
    using ComparisonResult = std::invoke_result_t<Comparer, LeftElementType, RightElementType>;

    ComparerCreatorSupplier creator_supplier_;
    std::shared_ptr<orders::TotalOrder<ComparisonResult> const> order_;
    std::shared_ptr<rcv_id_selectors::Selector<ComparisonResult> const> selector_;
    bool eq_max_;

public:
    BasicValueCalculator(
            ComparerCreatorSupplier creator_supplier,
            std::shared_ptr<orders::TotalOrder<ComparisonResult> const> order,
            std::shared_ptr<rcv_id_selectors::Selector<ComparisonResult> const> selector,
            std::optional<ComparisonResult> eq_value)
        : creator_supplier_(std::move(creator_supplier)),
          order_(std::move(order)),
          selector_(std::move(selector)),
          eq_max_(eq_value.has_value() ? *eq_value == order_->GreatestElement() : false) {}

    ComponentHandlingInfo Calculate(
            ParitioningValuesHolder<LeftElementType, RightElementType>&& values_holder,
            PartitionIndex::PositionListIndex const& right_pli,
            util::WorkerThreadPool* pool_ptr) const {
        if (eq_max_) {
            return BasicValueCalculatorCalc<ComparerCreatorSupplier, DecisionBoundaryType,
                                            Symmetric, true, EnableMultithreading>{
                    &creator_supplier_, order_.get(), selector_.get()}
                    .Calculate(std::move(values_holder), right_pli, pool_ptr);
        } else {
            return BasicValueCalculatorCalc<ComparerCreatorSupplier, DecisionBoundaryType,
                                            Symmetric, false, EnableMultithreading>{
                    &creator_supplier_, order_.get(), selector_.get()}
                    .Calculate(std::move(values_holder), right_pli, pool_ptr);
        }
    }

    std::string GetOrderName() const {
        return order_->ToString();
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
