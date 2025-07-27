#pragma once

#include <type_traits>

#include "algorithms/mde/decision_boundaries/float.h"
#include "algorithms/mde/decision_boundaries/signed_integer.h"
#include "algorithms/mde/decision_boundaries/unsigned_integer.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/pairwise.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/standard_calculator_creator.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/signed_integer.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/unsigned_integer.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/valid_float.h"
#include "algorithms/mde/hymde/utility/compile_time_value.h"

namespace algos::hymde::record_match_indexes::calculators {
namespace number_difference {
template <typename T>
T Difference(T a, T b) {
    return a - b;
}

template <typename OrderType>
struct Traits {
    using Order = OrderType;

    static constexpr auto kName = "diff";
    static constexpr Order::Type kEqValue = 0;
};
}  // namespace number_difference

template <typename OrderT, typename DecBoundType>
class NumberDifference
    : public NormalPairwise<number_difference::Traits<OrderT>,
                            number_difference::Difference<typename OrderT::Type>, DecBoundType,
                            utility::CompileTimeOptionalLike<typename OrderT::Type(0)>> {
public:
    using Creator = StandardCalculatorCreator<NumberDifference<OrderT, DecBoundType>>;

    using NormalPairwise<
            number_difference::Traits<OrderT>, number_difference::Difference<typename OrderT::Type>,
            DecBoundType,
            utility::CompileTimeOptionalLike<typename OrderT::Type(0)>>::NormalPairwise;
};

using FloatDifference =
        NumberDifference<orders::ValidFloat, model::mde::decision_boundaries::Float>;
using IntDifference =
        NumberDifference<orders::SignedInteger, model::mde::decision_boundaries::SignedInteger>;
}  // namespace algos::hymde::record_match_indexes::calculators
