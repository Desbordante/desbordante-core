#pragma once

#include "algorithms/dc/FastADC/fastadc.h"
#include "all_csv_configs.h"
#include "benchmark_comparer.h"
#include "benchmark_runner.h"

namespace benchmark {

inline void ADCBenchmark(BenchmarkRunner& runner, [[maybe_unused]] BenchmarkComparer& comparer) {
    runner.RegisterSimpleBenchmark<algos::dc::FastADC>(tests::kNeighbors100k);
}

}  // namespace benchmark
