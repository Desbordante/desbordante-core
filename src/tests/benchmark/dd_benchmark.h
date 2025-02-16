#pragma once

#include "algorithms/dd/split/split.h"
#include "all_csv_configs.h"
#include "benchmark_runner.h"

namespace benchmark {

inline void DDBenchmark(BenchmarkRunner& runner) {
    runner.RegisterSimpleBenchmark<algos::dd::Split>(tests::kAdult9attr);
}

}  // namespace benchmark
