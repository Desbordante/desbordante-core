#pragma once

#include "algorithms/fd/aidfd/aid.h"
#include "algorithms/fd/eulerfd/eulerfd.h"
#include "algorithms/fd/hyfd/hyfd.h"
#include "algorithms/fd/pyro/pyro.h"
#include "algorithms/fd/tane/tane.h"
#include "all_csv_configs.h"
#include "config/max_lhs/type.h"
#include "config/names.h"
#include "config/thread_number/type.h"
#include "performance_testing.h"

namespace perf_tests {

inline void FDPerfTests() {
    using namespace config::names;

    auto& testing = PerformanceTesting::Instance();

    testing.RegisterSimpleTest<algos::hyfd::HyFD>(
            tests::kIowa450k,
            {{kThreads, static_cast<config::ThreadNumType>(1)},
             {kMaximumLhs, static_cast<config::MaxLhsType>(2)}},
            "", 15);

    testing.RegisterSimpleTest<algos::Pyro>(
            tests::kIowa450k,
            {{kError, static_cast<config::ErrorType>(0.0)},
             {kSeed, static_cast<decltype(algos::pyro::Parameters::seed)>(0)},
             {kMaximumLhs, static_cast<config::MaxLhsType>(2)},
             {kThreads, static_cast<config::ThreadNumType>(1)}},
            "", 15);

    for (auto measure : algos::AfdErrorMeasure::_values()) {
        // mu_plus is much slower than other measures
        auto const& dataset = measure == +algos::AfdErrorMeasure::mu_plus
                                      ? tests::kMushroomPlus3attr1k
                                      : tests::kMushroomPlus4attr1k;
        testing.RegisterSimpleTest<algos::Tane>(
                dataset,
                {{kError, static_cast<config::ErrorType>(0.95)}, {kAfdErrorMeasure, measure}},
                measure._to_string(), 15);
    }

    // EulerFD tests are currently disabled, since test below runs less than a second, and
    // for reasonable run times we need EulerFD options, which are not implemented yet
    // TODO(senichenkov): implement EulerFD tests
#if 0
    testing.RegisterSimpleTest<algos::EulerFD>(tests::kBreastCancer,
                                               {{kCustomRandom, std::optional<int>{47}}}, "", 20);
#endif

    testing.RegisterSimpleTest<algos::Aid>(tests::kIowa1kk);
}

}  // namespace perf_tests
