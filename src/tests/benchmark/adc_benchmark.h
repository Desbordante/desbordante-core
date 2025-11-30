#pragma once

#include "core/algorithms/dc/FastADC/fastadc.h"
#include "tests/benchmark/benchmark_comparer.h"
#include "tests/benchmark/benchmark_runner.h"
#include "tests/common/all_csv_configs.h"

namespace benchmark {

inline void ADCBenchmark(BenchmarkRunner& runner, [[maybe_unused]] BenchmarkComparer& comparer) {
    runner.RegisterSimpleBenchmark<algos::dc::FastADC>(tests::kNeighbors100k);
}

}  // namespace benchmark
