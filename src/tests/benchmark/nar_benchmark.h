#pragma once

#include <cmath>

#include "core/algorithms/nar/des/des.h"
#include "core/config/names.h"
#include "tests/benchmark/benchmark_comparer.h"
#include "tests/benchmark/benchmark_runner.h"
#include "tests/common/all_csv_configs.h"

namespace benchmark {

inline void NARBenchmark(BenchmarkRunner& runner, BenchmarkComparer& comparer) {
    using namespace config::names;

    auto const des_name = runner.RegisterSimpleBenchmark<algos::des::DES>(
            tests::kAdult, {{kMinimumSupport, 0.0},
                            {kMinimumConfidence, 0.0},
                            {kPopulationSize, static_cast<unsigned>(std::pow(10, 5))},
                            {kMaxFitnessEvaluations, static_cast<unsigned>(std::pow(10, 5))},
                            {kCrossoverProbability, 0.9},
                            {kDifferentialScale, 0.5},
                            {kDifferentialStrategy, +algos::des::DifferentialStrategy::rand1Bin}});
    comparer.SetThreshold(des_name, 20);
}

}  // namespace benchmark
