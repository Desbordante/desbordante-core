#pragma once

#include "algorithms/mde/hymde/record_match_indexes/calculators/basic_partitioner.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/basic_value_calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator_impl.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"

namespace algos::hymde::record_match_indexes::calculators {
namespace pairwise {
template <auto Function>
class BasicComparerCreator {
    using FuncType = decltype(Function);
    using PartitioningValueLeft = std::remove_cvref_t<util::ArgumentType<FuncType, 0>>;
    using PartitioningValueRight = std::remove_cvref_t<util::ArgumentType<FuncType, 1>>;
    using ComparisonResult =
            std::invoke_result_t<FuncType, PartitioningValueLeft, PartitioningValueRight>;
    using Order = orders::TotalOrder<ComparisonResult>;

    class Comparer {
        ComparisonResult* cutoff_;
        Order* order_;
        ComparisonResult least_element_ = order_->LeastElement();

    public:
        Comparer(ComparisonResult* cutoff, Order* order) : cutoff_(cutoff), order_(order) {}

        ComparisonResult operator()(PartitioningValueLeft const& l,
                                    PartitioningValueRight const& r) {
            ComparisonResult res = Function(l, r);
            return order_->AreInOrder(*cutoff_, res) ? res : least_element_;
        }
    };

    ComparisonResult* cutoff_;
    Order* order_;

public:
    explicit BasicComparerCreator(ComparisonResult* cutoff, Order* order)
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

    auto operator()(std::vector<PartitioningValueLeft> const*,
                    std::vector<PartitioningValueRight> const*) const {
        return BasicComparerCreator<Function>{cutoff_, order_.get()};
    }
};

template <auto Function>
using Partitioner =
        BasicPartitionCalculator<std::remove_cvref_t<util::ArgumentType<decltype(Function), 0>>,
                                 std::remove_cvref_t<util::ArgumentType<decltype(Function), 1>>>;

template <auto Function, typename DecisionBoundaryType, bool... Params>
using PairwiseBase = CalculatorImpl<Partitioner<Function>,
                                    BasicValueCalculator<BasicComparerCreatorSupplier<Function>,
                                                         DecisionBoundaryType, Params...>>;

template <auto Function>
using ComparisonResult =
        typename pairwise::BasicComparerCreatorSupplier<Function>::ComparisonResult;
}  // namespace pairwise

template <typename CalcClass, auto Function, typename DecisionBoundaryType, bool Symmetric,
          bool... Params>
class Pairwise
    : public pairwise::PairwiseBase<Function, DecisionBoundaryType, Symmetric, Params...> {
    using Base = pairwise::PairwiseBase<Function, DecisionBoundaryType, Symmetric, Params...>;
    using ComparerCreatorSupplier = pairwise::BasicComparerCreatorSupplier<Function>;
    using Partitioner = pairwise::Partitioner<Function>;

public:
    using PartitioningValueLeft = ComparerCreatorSupplier::PartitioningValueLeft;
    using PartitioningValueRight = ComparerCreatorSupplier::PartitioningValueRight;
    using ComparisonResult = ComparerCreatorSupplier::ComparisonResult;

    using OrderPtr = std::shared_ptr<typename ComparerCreatorSupplier::Order>;
    using SelectorPtr = std::shared_ptr<rcv_id_selectors::Selector<ComparisonResult> const>;
    using PartitioningFunctionsOption = Partitioner::PartitioningFunctionsOption;

    Pairwise(records::DictionaryCompressed const& records,
             PartitioningFunctionsOption partitioning_functions, OrderPtr order,
             SelectorPtr selector, ComparisonResult cutoff)
        : Base(CalcClass::kName, Partitioner{std::move(partitioning_functions), records},
               {ComparerCreatorSupplier{cutoff, order}, order, std::move(selector),
                CalcClass::kEqValue}) {};
};

template <typename CalcClass, auto Function, typename DecisionBoundaryType>
using NormalPairwise = Pairwise<CalcClass, Function, DecisionBoundaryType, true, true>;
}  // namespace algos::hymde::record_match_indexes::calculators
