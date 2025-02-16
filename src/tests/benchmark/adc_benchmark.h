#pragma once

#include "algorithms/dc/FastADC/fastadc.h"
#include "all_csv_configs.h"
#include "benchmark_runner.h"

namespace benchmark {

inline void ADCBenchmark(BenchmarkRunner& runner) {
    runner.RegisterSimpleBenchmark<algos::dc::FastADC>(tests::kNeighbors120k);
}

}  // namespace benchmark
