#pragma once

#include <memory>
#include <utility>

#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/creator_base.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"
#include "algorithms/mde/hymde/record_match_indexes/partitioning_functions/partitioning_function.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/selector.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/size_based_selector_adapter.h"

namespace algos::hymde::record_match_indexes::calculators {
template <typename CalculatorType, typename... Params>
class StandardCalculatorCreator final : public CreatorBase<CalculatorType> {
    using Base = CreatorBase<CalculatorType>;

public:
    using PartitioningFunctionCreatorsOption = Base::PartitioningFunctionCreatorsOption;
    using ComparisonResult = CalculatorType::ComparisonResult;
    using OrderPtr = CalculatorType::OrderPtr;
    using SelectorPtr = std::shared_ptr<rcv_id_selectors::Selector<ComparisonResult> const>;

private:
    [[no_unique_address]] std::tuple<Params...> additional_params_;
    OrderPtr order_ptr_;
    SelectorPtr selector_ptr_;
    ComparisonResult cutoff_;

public:
    StandardCalculatorCreator(PartitioningFunctionCreatorsOption pf_creators, OrderPtr order_ptr,
                              SelectorPtr selector_ptr, ComparisonResult cutoff,
                              Params... additional_params)
        : Base(std::move(pf_creators)),
          additional_params_(std::move(additional_params)...),
          order_ptr_(std::move(order_ptr)),
          selector_ptr_(std::move(selector_ptr)),
          cutoff_(std::move(cutoff)) {}

    StandardCalculatorCreator(
            PartitioningFunctionCreatorsOption pf_creators, OrderPtr order_ptr,
            std::shared_ptr<rcv_id_selectors::SizeBasedSelector const> selector_ptr,
            ComparisonResult cutoff)
        : StandardCalculatorCreator(
                  std::move(pf_creators), std::move(order_ptr),
                  std::make_shared<
                          rcv_id_selectors::SizeBasedSelectorAdapter<ComparisonResult> const>(
                          std::move(selector_ptr)),
                  std::move(cutoff)) {}

    std::unique_ptr<Calculator> Create(RelationalSchema const& left_schema,
                                       RelationalSchema const& right_schema,
                                       records::DictionaryCompressed const& records) const final {
        return std::unique_ptr<Calculator>(static_cast<Calculator*>(
                std::apply(
                        [&](auto&&... additional_params) {
                            return std::make_unique<CalculatorType>(
                                    records,
                                    Base::MakePartFuncs(left_schema, right_schema, records),
                                    order_ptr_, selector_ptr_, cutoff_, additional_params...);
                        },
                        additional_params_)
                        .release()));
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
