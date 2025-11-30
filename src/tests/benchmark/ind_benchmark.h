#pragma once

#include <cmath>
#include <string>

#include "core/algorithms/ind/faida/faida.h"
#include "core/algorithms/ind/mind/mind.h"
#include "core/config/error/type.h"
#include "core/config/names.h"
#include "core/config/thread_number/type.h"
#include "tests/benchmark/benchmark_comparer.h"
#include "tests/benchmark/benchmark_runner.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace benchmark {

inline void INDBenchmark(BenchmarkRunner& runner, [[maybe_unused]] BenchmarkComparer& comparer) {
    using namespace config::names;

// FAIDA tests are currently disabled, since test below runs 10 seconds, and for reasonable
// run times we need really huge dataset
#if 0
    auto faida_test = [] {
        algos::StdParamsMap params{
                {kCsvConfigs,
                 tests::CSVConfigs{tests::kIowa1kk, tests::kMushroom, tests::kBreastCancer}},
                // Description says "higher sample size -- higher memory
                // consumption", but I don't know how it affects time
                {kSampleSize, 500},
                // Closer to 0 -- higher memory consumption and slower algo
                // Not every value is accepted
                {kHllAccuracy, std::pow(10, -4)},
                {kThreads, static_cast<config::ThreadNumType>(1)}};
        auto algo = algos::CreateAndLoadAlgorithm<algos::Faida>(params);
        algo->Execute();
    };
    std::string const faida_name = "Faida, Iowa1kk, mushroom, breast_cancer";
    runner.RegisterBenchmark(faida_name, std::move(faida_test));
    comparer.SetThreshold(faida_name, 20);
#endif

    auto mind_bm = [] {
        algos::StdParamsMap params{{kCsvConfigs, tests::CSVConfigs{tests::kIowa550k}},
                                   {kError, static_cast<config::ErrorType>(0.8)}};
        auto algo = algos::CreateAndLoadAlgorithm<algos::Mind>(params);
        algo->Execute();
    };
    runner.RegisterBenchmark("Mind, Iowa550k", std::move(mind_bm));
}

}  // namespace benchmark
