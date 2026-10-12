#pragma once

#include "core/algorithms/od/fastod/fastod.h"
#include "tests/benchmark/benchmark_comparer.h"
#include "tests/benchmark/benchmark_runner.h"
#include "tests/common/all_csv_configs.h"

namespace benchmark {

inline void ODBenchmark(BenchmarkRunner& runner, [[maybe_unused]] BenchmarkComparer& comparer) {
    runner.RegisterSimpleBenchmark<algos::Fastod>(tests::kOdTestNormAbalone);
}

}  // namespace benchmark
