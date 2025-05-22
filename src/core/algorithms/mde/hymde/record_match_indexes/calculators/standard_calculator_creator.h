#pragma once

#include <memory>
#include <utility>

#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"
#include "algorithms/mde/hymde/record_match_indexes/partitioning_functions/partitioning_function.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/selector.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/size_based_selector_adapter.h"

namespace algos::hymde::record_match_indexes::calculators {
template <typename CalculatorType, typename... Params>
class StandardCalculatorCreator final : public Calculator::Creator {
    using PartitioningValueLeft = CalculatorType::PartitioningValueLeft;
    using PartitioningValueRight = CalculatorType::PartitioningValueRight;
    using PartitioningFunctionLeft =
            partitioning_functions::PartitioningFunction<PartitioningValueLeft>;
    using PartitioningFunctionRight =
            partitioning_functions::PartitioningFunction<PartitioningValueRight>;
    using PFCreatorPtrL = std::shared_ptr<typename PartitioningFunctionLeft::Creator>;
    using PFCreatorPtrR = std::shared_ptr<typename PartitioningFunctionRight::Creator>;
    using PFCreatorPtrPair = std::pair<PFCreatorPtrL, PFCreatorPtrR>;

    constexpr static bool kPartValuesAreSame =
            std::is_same_v<PartitioningValueLeft, PartitioningValueRight>;

public:
    using PartitioningFunctionCreatorsOption =
            std::conditional_t<kPartValuesAreSame, std::variant<PFCreatorPtrL, PFCreatorPtrPair>,
                               PFCreatorPtrPair>;
    using ComparisonResult = CalculatorType::ComparisonResult;
    using OrderPtr = CalculatorType::OrderPtr;
    using SelectorPtr = std::shared_ptr<rcv_id_selectors::Selector<ComparisonResult> const>;

private:
    PartitioningFunctionCreatorsOption pf_creators_;
    [[no_unique_address]] std::tuple<Params...> additional_params_;
    OrderPtr order_ptr_;
    SelectorPtr selector_ptr_;
    ComparisonResult cutoff_;

    static auto MakePair(PFCreatorPtrL const& pf_creator_left,
                         PFCreatorPtrR const& pf_creator_right, RelationalSchema const& left_schema,
                         RelationalSchema const& right_schema,
                         records::DictionaryCompressed const& records) {
        return std::pair{pf_creator_left->Create(left_schema, records.GetValues()),
                         pf_creator_right->Create(right_schema, records.GetValues())};
    }

    static auto MakeSingle(PFCreatorPtrL const& pf_creator, RelationalSchema const& schema,
                           records::DictionaryCompressed const& records) {
        return pf_creator->Create(schema, records.GetValues());
    }

    auto MakePartFuncs(RelationalSchema const& left_schema, RelationalSchema const& right_schema,
                       records::DictionaryCompressed const& records) const {
        if constexpr (kPartValuesAreSame) {
            using SingleResult =
                    std::invoke_result_t<decltype(&StandardCalculatorCreator::MakeSingle),
                                         PFCreatorPtrL const&, RelationalSchema const&,
                                         records::DictionaryCompressed const&>;
            using PairResult =
                    std::invoke_result_t<decltype(&StandardCalculatorCreator::MakePair),
                                         PFCreatorPtrL const&, PFCreatorPtrR const&,
                                         RelationalSchema const&, RelationalSchema const&,
                                         records::DictionaryCompressed const&>;
            using OptionType = std::variant<SingleResult, PairResult>;

            if (PFCreatorPtrL const* creator_ptr_ptr = std::get_if<PFCreatorPtrL>(&pf_creators_)) {
                if (&left_schema == &right_schema)
                    return OptionType{MakeSingle(*creator_ptr_ptr, left_schema, records)};
                return OptionType{MakePair(*creator_ptr_ptr, *creator_ptr_ptr, left_schema,
                                           right_schema, records)};
            } else {
                DESBORDANTE_ASSUME(std::holds_alternative<PFCreatorPtrPair>(pf_creators_));
                auto const& [left, right] = std::get<PFCreatorPtrPair>(pf_creators_);
                return OptionType{MakePair(left, right, left_schema, right_schema, records)};
            }
        } else {
            auto const& [left, right] = pf_creators_;
            return MakePair(left, right, left_schema, right_schema, records);
        }
    }

public:
    StandardCalculatorCreator(PartitioningFunctionCreatorsOption pf_creators, OrderPtr order_ptr,
                              SelectorPtr selector_ptr, ComparisonResult cutoff,
                              Params... additional_params)
        : pf_creators_(std::move(pf_creators)),
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
                                    records, MakePartFuncs(left_schema, right_schema, records),
                                    order_ptr_, selector_ptr_, cutoff_, additional_params...);
                        },
                        additional_params_)
                        .release()));
    }

    void CheckSchemas(RelationalSchema const& left_schema,
                      RelationalSchema const& right_schema) const final {
        if constexpr (kPartValuesAreSame) {
            if (PFCreatorPtrL const* creator_ptr_ptr = std::get_if<PFCreatorPtrL>(&pf_creators_)) {
                (*creator_ptr_ptr)->CheckSchema(left_schema);
                (*creator_ptr_ptr)->CheckSchema(right_schema);
            } else {
                DESBORDANTE_ASSUME(std::holds_alternative<PFCreatorPtrPair>(pf_creators_));
                auto const& [left, right] = std::get<PFCreatorPtrPair>(pf_creators_);
                left->CheckSchema(left_schema);
                right->CheckSchema(right_schema);
            }
        } else {
            auto const& [left, right] = pf_creators_;
            left->CheckSchema(left_schema);
            right->CheckSchema(right_schema);
        }
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
