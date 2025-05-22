#pragma once

#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/build_indexes.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/encode_results.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/meaningful_table_results.h"
#include "algorithms/mde/hymde/record_match_indexes/indexes.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/selector.h"
#include "algorithms/mde/hymde/utility/compile_time_value.h"
#include "algorithms/mde/hymde/utility/index_range.h"
#include "util/argument_type.h"
#include "util/worker_thread_pool.h"

namespace algos::hymde::record_match_indexes::calculators {
template <typename ComparerCreatorSupplier, typename DecisionBoundaryType, typename EqValue,
          typename Symmetric, typename EnableMultithreading = utility::CompileTimeValue<true>>
class ValueCalculator {
    // TODO: less type deduction

    using ComparisonResult = DecisionBoundaryType::ValueType;

    using OrderPtr = std::shared_ptr<orders::TotalOrder<ComparisonResult> const>;
    using SelectorPtr = std::shared_ptr<rcv_id_selectors::Selector<ComparisonResult> const>;

    [[no_unique_address]] EqValue eq_value_;
    [[no_unique_address]] Symmetric symmetric_;
    [[no_unique_address]] EnableMultithreading enable_multithreading_;
    // May store the comparison function.
    ComparerCreatorSupplier creator_supplier_;
    OrderPtr order_;
    SelectorPtr selector_;

    constexpr bool Multithreaded() const noexcept {
        return enable_multithreading_.value;
    }

    template <typename Comparer>
    struct ThreadResource {
        Comparer comparer;
        bool least_element_found = false;
    };

    template <bool UseSameMethod, typename ComparerCreator, typename LeftElementType,
              typename RightElementType>
    class Worker {
        using LeftValueComparisonInfoOrigType = LeftValueComparisonInfo<ComparisonResult>;
        std::vector<LeftElementType> const& left_elements_;
        std::vector<RightElementType> const& right_elements_;
        PartitionIndex::PositionListIndex const& right_pli_;
        orders::TotalOrder<ComparisonResult> const& order_;
        ComparerCreator create_comparer_;
        [[no_unique_address]] EqValue eq_value_;
        [[no_unique_address]] Symmetric symmetric_;

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

        void CalcOnePair(auto& comparer, LeftValueComparisonInfoOrigType& lv_comp_info,
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

        void CalcLoop(auto& comparer, LeftValueComparisonInfoOrigType& lv_comp_info,
                      LeftElementType const& left_element, bool& least_element_found,
                      PartitionValueId from, PartitionValueId to) {
            for (PartitionValueId pvalue_id_right : std::views::iota(from, to)) {
                CalcOnePair(comparer, lv_comp_info, left_element, pvalue_id_right,
                            least_element_found);
            }
        }

        void CalcForFull(auto& comparer, PartitionValueId pvalue_id_left,
                         bool& least_element_found) {
            LeftElementType const& left_element = left_elements_[pvalue_id_left];
            LeftValueComparisonInfoOrigType& lv_comp_info = task_data_[pvalue_id_left];
            CalcLoop(comparer, lv_comp_info, left_element, least_element_found, 0,
                     num_values_right_);
        }

        void CalcForSame(auto& comparer, PartitionValueId pvalue_id_left,
                         bool& least_element_found) {
            LeftElementType const& left_element = left_elements_[pvalue_id_left];
            LeftValueComparisonInfoOrigType& lv_comp_info = task_data_[pvalue_id_left];
            if (!FunctionIsSymmetric()) {
                CalcLoop(comparer, lv_comp_info, left_element, least_element_found, 0,
                         pvalue_id_left);
            }
            if (EqHasKnownValue()) {
                AddRightPartValIdCompResPair(lv_comp_info, pvalue_id_left, GetEqValue());
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

        auto AcquireResource() {
            return ThreadResource{create_comparer_()};
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
            auto [comparer, least_element_found] = AcquireResource();
            auto calc_same = [&]() {
                for (PartitionValueId left_pvalue_id : utility::IndexRange(num_values_left_)) {
                    task_data_.emplace_back();
                    CalcForSame(comparer, left_pvalue_id, least_element_found);
                }
            };
            auto calc_full = [&]() {
                for (PartitionValueId left_pvalue_id : utility::IndexRange(num_values_left_)) {
                    task_data_.emplace_back();
                    CalcForFull(comparer, left_pvalue_id, least_element_found);
                }
            };
            if constexpr (UseSameMethod) {
                if (OneFunctionForOneTableGiven()) {
                    calc_same();
                } else {
                    calc_full();
                }
            } else {
                calc_full();
            }
            return Enumerate(least_element_found);
        }

        auto ExecMultiThreaded(util::WorkerThreadPool& pool) {
            task_data_.assign(num_values_left_, {});
            std::atomic<bool> least_element_found = false;
            auto set_least_element = [&least_element_found](auto resource) {
                if (resource.least_element_found)
                    least_element_found.store(true, std::memory_order::release);
            };
            auto acquire_resource = [this]() { return AcquireResource(); };
            auto calc_full = [this](PartitionValueId left_pvalue_id, auto& resource) {
                CalcForFull(resource.comparer, left_pvalue_id, resource.least_element_found);
            };
            auto calc_same = [this](PartitionValueId left_pvalue_id, auto& resource) {
                CalcForSame(resource.comparer, left_pvalue_id, resource.least_element_found);
            };
            if constexpr (UseSameMethod) {
                if (OneFunctionForOneTableGiven()) {
                    pool.ExecIndexWithResource(calc_same, acquire_resource, num_values_left_,
                                               set_least_element);
                } else {
                    pool.ExecIndexWithResource(calc_full, acquire_resource, num_values_left_,
                                               set_least_element);
                }
            } else {
                pool.ExecIndexWithResource(calc_full, acquire_resource, num_values_left_,
                                           set_least_element);
            }
            return Enumerate(least_element_found.load(std::memory_order::acquire));
        }

        constexpr bool FunctionIsSymmetric() const noexcept {
            return symmetric_.value;
        }

        constexpr bool EqHasKnownValue() const noexcept {
            return eq_value_.value.has_value();
        }

        constexpr auto GetEqValue() const noexcept {
            return *eq_value_.value;
        }
    };

    template <typename ComparerCreator, typename LeftElementType, typename RightElementType>
    class SameMethodWorker
        : public Worker<true, ComparerCreator, LeftElementType, RightElementType> {
    public:
        SameMethodWorker(std::vector<LeftElementType> const* left_elements,
                         std::vector<RightElementType> const* right_elements,
                         PartitionIndex::PositionListIndex const& right_clusters,
                         orders::TotalOrder<ComparisonResult> const& order,
                         ComparerCreator create_comparer)
            : Worker<true, ComparerCreator, LeftElementType, RightElementType>(
                      left_elements, right_elements, right_clusters, order,
                      std::move(create_comparer)) {}
    };

    template <typename ComparerCreator, typename LeftElementType, typename RightElementType>
    class PairWorker : public Worker<false, ComparerCreator, LeftElementType, RightElementType> {
    public:
        PairWorker(std::vector<LeftElementType> const* left_elements,
                   std::vector<RightElementType> const* right_elements,
                   PartitionIndex::PositionListIndex const& right_clusters,
                   orders::TotalOrder<ComparisonResult> const& order,
                   ComparerCreator create_comparer)
            : Worker<false, ComparerCreator, LeftElementType, RightElementType>(
                      left_elements, right_elements, right_clusters, order,
                      std::move(create_comparer)) {}
    };

public:
    ValueCalculator(EqValue eq_value, Symmetric symmetric,
                    EnableMultithreading enable_multithreading,
                    ComparerCreatorSupplier creator_supplier, OrderPtr order, SelectorPtr selector)
        : eq_value_(std::move(eq_value)),
          symmetric_(std::move(symmetric)),
          enable_multithreading_(std::move(enable_multithreading)),
          creator_supplier_(std::move(creator_supplier)),
          order_(std::move(order)),
          selector_(std::move(selector)) {}

    auto CalculateSingle(auto&& part, PartitionIndex::PositionListIndex const& right_pli,
                         util::WorkerThreadPool* pool_ptr) const {
        auto const* left_elements = part.GetElements();
        auto const* right_elements = left_elements;
        auto create_comparer = creator_supplier_(part);
        SameMethodWorker worker{left_elements, right_elements, right_pli, *order_,
                                std::move(create_comparer)};
        auto [classifier_values, enumerated_results, least_element_found] =
                enable_multithreading_.value && pool_ptr != nullptr
                        ? worker.ExecMultiThreaded(*pool_ptr)
                        : worker.ExecSingleThreaded();
        if (worker.FunctionIsSymmetric()) {
            SymmetricClosure(enumerated_results, right_pli);
        }

        bool const eq_is_greatest =
                worker.EqHasKnownValue() && worker.GetEqValue() == order_->GreatestElement();
        ComponentStructureAssertions assertions{
                .assume_overlap_lpli_cluster_max_ = eq_is_greatest,
                .assume_record_symmetric_ = worker.FunctionIsSymmetric()};
        return BuildIndexes<DecisionBoundaryType>(std::move(enumerated_results),
                                                  std::move(classifier_values), right_pli,
                                                  *selector_, least_element_found, assertions);
    }

    auto CalculatePair(auto&& part, PartitionIndex::PositionListIndex const& right_pli,
                       util::WorkerThreadPool* pool_ptr) const {
        auto const* left_elements = part.GetLeftPtr();
        auto const* right_elements = part.GetRightPtr();
        auto create_comparer = creator_supplier_(part);
        PairWorker worker{left_elements, right_elements, right_pli, *order_,
                          std::move(create_comparer)};
        auto [classifier_values, enumerated_results, least_element_found] =
                enable_multithreading_.value && pool_ptr != nullptr
                        ? worker.ExecMultiThreaded(*pool_ptr)
                        : worker.ExecSingleThreaded();

        ComponentStructureAssertions assertions{.assume_overlap_lpli_cluster_max_ = false,
                                                .assume_record_symmetric_ = false};
        return BuildIndexes<DecisionBoundaryType>(std::move(enumerated_results),
                                                  std::move(classifier_values), right_pli,
                                                  *selector_, least_element_found, assertions);
    }

    std::string GetOrderName() const {
        return order_->ToString();
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
