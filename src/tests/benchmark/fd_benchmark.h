#pragma once

#include <magic_enum/magic_enum.hpp>

#include "algorithms/fd/aidfd/aid.h"
#include "algorithms/fd/eulerfd/eulerfd.h"
#include "algorithms/fd/hyfd/hyfd.h"
#include "algorithms/fd/pyro/pyro.h"
#include "algorithms/fd/tane/tane.h"
#include "all_csv_configs.h"
#include "benchmark_comparer.h"
#include "benchmark_runner.h"
#include "config/max_lhs/type.h"
#include "config/names.h"
#include "config/thread_number/type.h"

namespace benchmark {

inline void FDBenchmark(BenchmarkRunner& runner, BenchmarkComparer& comparer) {
    using namespace config::names;

    // HyFD became less "stable" after merging #580, so for now threshold is 70%
    auto hyfd_name = runner.RegisterSimpleBenchmark<algos::hyfd::HyFD>(
            tests::kIowa650k,
            {{kThreads, static_cast<config::ThreadNumType>(1)},
             {kMaximumLhs, static_cast<config::MaxLhsType>(2)}},
            "");
    comparer.SetThreshold(hyfd_name, 70);

    auto pyro_name = runner.RegisterSimpleBenchmark<algos::Pyro>(
            tests::kIowa550k,
            {{kError, static_cast<config::ErrorType>(0.0)},
             {kSeed, static_cast<decltype(algos::pyro::Parameters::seed)>(0)},
             {kMaximumLhs, static_cast<config::MaxLhsType>(2)},
             {kThreads, static_cast<config::ThreadNumType>(1)}},
            "");
    comparer.SetThreshold(pyro_name, 17);

    for (auto measure : magic_enum::enum_values<algos::AfdErrorMeasure>()) {
        // mu_plus is much slower than other measures
        auto const& dataset = measure == algos::AfdErrorMeasure::kMuPlus
                                      ? tests::kMushroomPlus3attr1500
                                      : tests::kMushroomPlus4attr1300;
        auto tane_name = runner.RegisterSimpleBenchmark<algos::Tane>(
                dataset,
                {{kError, static_cast<config::ErrorType>(0.95)}, {kAfdErrorMeasure, measure}},
                std::string(magic_enum::enum_name(measure)));
        comparer.SetThreshold(tane_name, 20);
    }

// EulerFD tests are currently disabled, since test below runs less than a second, and
// for reasonable run times we need EulerFD options, which are not implemented yet
#if 0
    auto eulerfd_name = testing.RegisterSimpleTest<algos::EulerFD>(
            tests::kBreastCancer, {{kCustomRandom, std::optional<int>{47}}}, "", 20);
    comparer.SetThreshold(eulerfd_name, 20);
#endif

    auto aid_name = runner.RegisterSimpleBenchmark<algos::Aid>(tests::kIowa1kk, {}, "");
    comparer.SetThreshold(aid_name, 17);
}

}  // namespace benchmark
