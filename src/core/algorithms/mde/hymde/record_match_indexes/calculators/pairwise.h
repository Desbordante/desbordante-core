#pragma once

#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator_impl.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/partition_builder_supplier.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/value_calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"
#include "algorithms/mde/hymde/utility/compile_time_value.h"

namespace algos::hymde::record_match_indexes::calculators {
namespace pairwise {
template <auto Function>
class BasicComparerCreator {
public:
    using FuncType = decltype(Function);
    using PartitioningValueLeft = std::remove_cvref_t<util::ArgumentType<FuncType, 0>>;
    using PartitioningValueRight = std::remove_cvref_t<util::ArgumentType<FuncType, 1>>;
    using ComparisonResult =
            std::invoke_result_t<FuncType, PartitioningValueLeft, PartitioningValueRight>;
    using Order = orders::TotalOrder<ComparisonResult>;

    class Comparer {
        ComparisonResult const* cutoff_;
        Order* order_;
        ComparisonResult least_element_ = order_->LeastElement();

    public:
        Comparer(ComparisonResult const* cutoff, Order* order) : cutoff_(cutoff), order_(order) {}

        ComparisonResult operator()(PartitioningValueLeft const& l,
                                    PartitioningValueRight const& r) {
            ComparisonResult res = Function(l, r);
            return order_->AreInOrder(*cutoff_, res) ? res : least_element_;
        }
    };

private:
    ComparisonResult const* cutoff_;
    Order* order_;

public:
    explicit BasicComparerCreator(ComparisonResult const* cutoff, Order* order)
        : cutoff_(cutoff), order_(order) {}

    Comparer operator()() const {
        return {cutoff_, order_};
    }
};

template <auto Function>
class BasicComparerCreatorSupplier {
    using ComparerCreator = BasicComparerCreator<Function>;

public:
    using PartitioningValueLeft = ComparerCreator::PartitioningValueLeft;
    using PartitioningValueRight = ComparerCreator::PartitioningValueRight;
    using ComparisonResult = ComparerCreator::ComparisonResult;
    using Order = ComparerCreator::Order;

private:
    ComparisonResult cutoff_;
    std::shared_ptr<Order> order_;

public:
    BasicComparerCreatorSupplier(ComparisonResult cutoff, std::shared_ptr<Order> order)
        : cutoff_(cutoff), order_(std::move(order)) {}

    auto operator()(auto&&) const {
        return BasicComparerCreator<Function>{&cutoff_, order_.get()};
    }
};

template <auto Function>
using BuilderSupplier = std::conditional_t<
        std::is_same_v<std::remove_cvref_t<util::ArgumentType<decltype(Function), 0>>,
                       std::remove_cvref_t<util::ArgumentType<decltype(Function), 1>>>,
        SameValueTypeBuilderSupplier<std::remove_cvref_t<util::ArgumentType<decltype(Function), 0>>,
                                     NullInspector, NullInspector>,
        PairOnlyBuilderSupplier<std::remove_cvref_t<util::ArgumentType<decltype(Function), 0>>,
                                std::remove_cvref_t<util::ArgumentType<decltype(Function), 1>>,
                                NullInspector>>;

template <auto Function, typename DecisionBoundaryType, typename... Params>
using PairwiseBase = CalculatorImpl<
        BuilderSupplier<Function>,
        ValueCalculator<BasicComparerCreatorSupplier<Function>, DecisionBoundaryType, Params...>>;

template <auto Function>
using ComparisonResult =
        typename pairwise::BasicComparerCreatorSupplier<Function>::ComparisonResult;
}  // namespace pairwise

template <typename Traits, auto Function, typename DecisionBoundaryType, typename... Params>
class Pairwise : public pairwise::PairwiseBase<Function, DecisionBoundaryType, Params...> {
    using Base = pairwise::PairwiseBase<Function, DecisionBoundaryType, Params...>;
    using ComparerCreatorSupplier = pairwise::BasicComparerCreatorSupplier<Function>;

public:
    using PartitioningValueLeft = ComparerCreatorSupplier::PartitioningValueLeft;
    using PartitioningValueRight = ComparerCreatorSupplier::PartitioningValueRight;
    using ComparisonResult = ComparerCreatorSupplier::ComparisonResult;

    using OrderPtr = std::shared_ptr<typename Traits::Order>;
    using SelectorPtr = std::shared_ptr<rcv_id_selectors::Selector<ComparisonResult> const>;
    using PartitioningFunctionsOption = Base::PartitioningFunctionsOption;

    Pairwise(records::DictionaryCompressed const& records,
             PartitioningFunctionsOption partitioning_functions, OrderPtr order,
             SelectorPtr selector, ComparisonResult cutoff, Params... params)
        : Base(Traits::kName, std::move(partitioning_functions), records, {},
               {std::move(params)..., ComparerCreatorSupplier{cutoff, order}, order,
                std::move(selector)}) {};
};

template <typename Traits, auto Function, typename DecisionBoundaryType, typename... Params>
class PairwiseCompileTime : public Pairwise<Traits, Function, DecisionBoundaryType, Params...> {
    static_assert((Params::kCompileTime && ...), "Params must be compile-time");

public:
    template <typename... PairwiseConstructorParams>
    PairwiseCompileTime(PairwiseConstructorParams&&... cparams)
        : Pairwise<Traits, Function, DecisionBoundaryType, Params...>(
                  std::forward<PairwiseConstructorParams>(cparams)..., Params{}...) {}
};

template <typename Traits, auto Function, typename DecisionBoundaryType, typename EqValue>
using NormalPairwise =
        PairwiseCompileTime<Traits, Function, DecisionBoundaryType, EqValue,
                            utility::CompileTimeValue<true>, utility::CompileTimeValue<true>>;
}  // namespace algos::hymde::record_match_indexes::calculators
