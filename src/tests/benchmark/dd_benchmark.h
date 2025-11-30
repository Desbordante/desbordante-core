#pragma once

#include "core/algorithms/dd/split/split.h"
#include "tests/benchmark/benchmark_comparer.h"
#include "tests/benchmark/benchmark_runner.h"
#include "tests/common/all_csv_configs.h"

namespace benchmark {

inline void DDBenchmark(BenchmarkRunner& runner, [[maybe_unused]] BenchmarkComparer& comparer) {
    runner.RegisterSimpleBenchmark<algos::dd::Split>(tests::kAdult9attr);
}

}  // namespace benchmark
