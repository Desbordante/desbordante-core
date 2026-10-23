#include "core/algorithms/pac/pac_verifier/options.h"

#include <cmath>
#include <functional>
#include <utility>

#include "core/config/common_option.h"
#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "core/config/option.h"

using namespace config;
using namespace config::names;
using namespace config::descriptions;

namespace {
using GetDouble = std::function<double()>;

constexpr static double kDefaultMinEpsilon = 0.0;
constexpr static double kDefaultMaxEpsilon = -1.0;
constexpr static double kDefaultMinDelta = 0.9;
constexpr static double kDefaultMaxDelta = 0.2;
// Diagonal threshold is the maximum slope coefficient of a segment on the ECDF, that is still
// considered horizontal during verifying PAC via elbow method
// See https://colab.research.google.com/drive/1t2i-BgzRaL3VSzL0Q0izR1RbgE1i0Ohu?usp=sharing
// 1e-5 is one extra value on a table containing 10^5 rows
constexpr static double kDefaultDiagonalThreshold = 1e-5;

void NotNegative(double value) {
    if (value < 0) {
        throw ConfigurationError("Min epsilon cannot be negative");
    }
}

Option<double>::ValueCheckFunc MinEpsLeMaxEps(GetDouble&& get_min_epsilon) {
    return [get_min_epsilon = std::move(get_min_epsilon)](double value) mutable {
        if (value > 0 && value < get_min_epsilon()) {
            throw config::ConfigurationError(
                    "Min epsilon must be less than or equal to max epsilon");
        }
    };
}

Option<double>::DefaultFunc CalculateDefaultMinDelta(GetDouble&& get_min_epsilon,
                                                     GetDouble&& get_max_epsilon) {
    return [get_min_epsilon = std::move(get_min_epsilon),
            get_max_epsilon = std::move(get_max_epsilon)]() mutable {
        if (get_min_epsilon() > 0 || get_max_epsilon() >= 0) {
            return 0.0;
        }
        return kDefaultMinDelta;
    };
}

void DeltaBounds(double value) {
    if (value > 1 || value < 0) {
        throw config::ConfigurationError("Min delta and max delta must be in [0, 1]");
    }
}

Option<double>::DefaultFunc CalculateDefaultMaxDelta(GetDouble&& get_min_epsilon,
                                                     GetDouble&& get_max_epsilon) {
    return [get_min_epsilon = std::move(get_min_epsilon),
            get_max_epsilon = std::move(get_max_epsilon)]() mutable {
        if (get_min_epsilon() > 0 || get_max_epsilon() >= 0) {
            return 1.0;
        }
        return kDefaultMaxDelta;
    };
}

Option<unsigned long>::DefaultFunc CalculateDefaultDeltaSteps(GetDouble&& get_min_delta,
                                                              GetDouble&& get_max_delta) {
    return [get_min_delta = std::move(get_min_delta),
            get_max_delta = std::move(get_max_delta)]() mutable {
        return std::round((get_min_delta() - get_max_delta()) * 1000);
    };
}
}  // namespace

namespace algos::pac_verifier {
extern CommonOption<double> const kMinEpsilonOption{kMinEpsilon, kDMinEpsilon, kDefaultMinEpsilon,
                                                    nullptr, NotNegative};

config::CommonOption<double> MaxEpsilonOption(GetDouble get_min_epsilon) {
    return CommonOption<double>{kMaxEpsilon, kDMaxEpsilon, kDefaultMaxEpsilon, nullptr,
                                MinEpsLeMaxEps(std::move(get_min_epsilon))};
}

config::CommonOption<double> MinDeltaOption(GetDouble get_min_epsilon, GetDouble get_max_epsilon) {
    return CommonOption<double>{
            kMinDelta, kDMinDelta,
            CalculateDefaultMinDelta(std::move(get_min_epsilon), std::move(get_max_epsilon)),
            nullptr, DeltaBounds};
}

config::CommonOption<double> MaxDeltaOption(GetDouble get_min_epsilon, GetDouble get_max_epsilon) {
    return CommonOption<double>{
            kMaxDelta, kDMaxDelta,
            CalculateDefaultMaxDelta(std::move(get_min_epsilon), std::move(get_max_epsilon)),
            nullptr, DeltaBounds};
}

config::CommonOption<unsigned long> DeltaStepsOption(GetDouble get_min_delta,
                                                     GetDouble get_max_delta) {
    return CommonOption<unsigned long>{
            kDeltaSteps, kDDeltaSteps,
            CalculateDefaultDeltaSteps(std::move(get_min_delta), std::move(get_max_delta))};
}

extern CommonOption<double> const kDiagonalThresholdOption{kDiagonalThreshold, kDDiagonalThreshold,
                                                           kDefaultDiagonalThreshold};
}  // namespace algos::pac_verifier
