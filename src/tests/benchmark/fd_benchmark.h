#pragma once

#include "algorithms/fd/aidfd/aid.h"
#include "algorithms/fd/eulerfd/eulerfd.h"
#include "algorithms/fd/hyfd/hyfd.h"
#include "algorithms/fd/pyro/pyro.h"
#include "algorithms/fd/tane/tane.h"
#include "all_csv_configs.h"
#include "benchmark_runner.h"
#include "config/max_lhs/type.h"
#include "config/names.h"
#include "config/thread_number/type.h"

namespace benchmark {

inline void FDBenchmark(BenchmarkRunner& runner) {
    using namespace config::names;

    runner.RegisterSimpleBenchmark<algos::hyfd::HyFD>(
            tests::kIowa650k,
            {{kThreads, static_cast<config::ThreadNumType>(1)},
             {kMaximumLhs, static_cast<config::MaxLhsType>(2)}},
            "", 25);

    runner.RegisterSimpleBenchmark<algos::Pyro>(
            tests::kIowa550k,
            {{kError, static_cast<config::ErrorType>(0.0)},
             {kSeed, static_cast<decltype(algos::pyro::Parameters::seed)>(0)},
             {kMaximumLhs, static_cast<config::MaxLhsType>(2)},
             {kThreads, static_cast<config::ThreadNumType>(1)}},
            "", 15);

    for (auto measure : algos::AfdErrorMeasure::_values()) {
        // mu_plus is much slower than other measures
        auto const& dataset = measure == +algos::AfdErrorMeasure::mu_plus
                                      ? tests::kMushroomPlus3attr1500
                                      : tests::kMushroomPlus4attr1300;
        runner.RegisterSimpleBenchmark<algos::Tane>(
                dataset,
                {{kError, static_cast<config::ErrorType>(0.95)}, {kAfdErrorMeasure, measure}},
                measure._to_string(), 15);
    }

    // EulerFD tests are currently disabled, since test below runs less than a second, and
    // for reasonable run times we need EulerFD options, which are not implemented yet
#if 0
    testing.RegisterSimpleTest<algos::EulerFD>(tests::kBreastCancer,
                                               {{kCustomRandom, std::optional<int>{47}}}, "", 20);
#endif

    runner.RegisterSimpleBenchmark<algos::Aid>(tests::kIowa1kk, {}, "", 17);
}

}  // namespace benchmark
