#pragma once

#include <type_traits>

#include "algorithms/mde/decision_boundaries/float.h"
#include "algorithms/mde/decision_boundaries/signed_integer.h"
#include "algorithms/mde/decision_boundaries/unsigned_integer.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/pairwise.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/standard_calculator_creator.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/non_negative_float.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/signed_integer.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/unsigned_integer.h"
#include "algorithms/mde/hymde/utility/compile_time_value.h"

namespace algos::hymde::record_match_indexes::calculators {
namespace number_distance {
template <typename T>
T DistanceImpl(T a, T b) {
    return b > a ? b - a : a - b;
}

template <typename T>
auto Distance(T a, T b) {
    if constexpr (std::is_integral_v<T>) {
        if constexpr (std::is_unsigned_v<T>) {
            return DistanceImpl(a, b);
        } else {
            using Unsigned = std::make_unsigned_t<T>;
            static constexpr Unsigned kAdd =
                    static_cast<Unsigned>(-(std::numeric_limits<T>::min() + 1)) + 1;
            Unsigned ua = kAdd + (a < 0 ? -(static_cast<Unsigned>(-(a + 1)) + 1u)
                                        : static_cast<Unsigned>(a));
            Unsigned ub = kAdd + (b < 0 ? -(static_cast<Unsigned>(-(b + 1)) + 1u)
                                        : static_cast<Unsigned>(b));
            return DistanceImpl(ua, ub);
        }
    } else {
        return DistanceImpl(a, b);
    }
}

template <typename OrderType>
struct Traits {
    using Order = OrderType;

    static constexpr auto kName = "dist";
};
}  // namespace number_distance

template <typename OrderT, typename DistValueType, typename DecBoundType>
class NumberDistance : public NormalPairwise<number_distance::Traits<OrderT>,
                                             number_distance::Distance<DistValueType>, DecBoundType,
                                             utility::CompileTimeOptionalLike<DistValueType(0)>> {
public:
    using Creator = StandardCalculatorCreator<NumberDistance<OrderT, DistValueType, DecBoundType>>;

    using NormalPairwise<number_distance::Traits<OrderT>, number_distance::Distance<DistValueType>,
                         DecBoundType,
                         utility::CompileTimeOptionalLike<DistValueType(0)>>::NormalPairwise;
};

using FloatDistance =
        NumberDistance<orders::NonNegativeFloat, double, model::mde::decision_boundaries::Float>;
using IntDistance = NumberDistance<orders::UnsignedInteger, std::intmax_t,
                                   model::mde::decision_boundaries::UnsignedInteger>;
}  // namespace algos::hymde::record_match_indexes::calculators
