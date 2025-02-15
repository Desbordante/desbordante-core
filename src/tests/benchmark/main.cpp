#include <cstdlib>
#include <iostream>
#include <unordered_map>

#include <easylogging++.h>

#include "adc_benchmark.h"
#include "benchmark_comparer.h"
#include "benchmark_results_io.h"
#include "benchmark_runner.h"
#include "dd_benchmark.h"
#include "fd_benchmark.h"
#include "ind_benchmark.h"
#include "md_benchmark.h"
#include "nar_benchmark.h"

INITIALIZE_EASYLOGGINGPP

// Expected arguments (both are optional):
// 1. file with previous benchmark results
// 2. file to save current benchmark results
int main(int argc, char* argv[]) {
    using namespace benchmark;

    el::Loggers::configureFromGlobal("logging.conf");

    BenchmarkRunner bm_runner;
    for (auto test_register_func :
         {ADCBenchmark, DDBenchmark, INDBenchmark, FDBenchmark, MDBenchmark, NARBenchmark}) {
        test_register_func(bm_runner);
    }
    bm_runner.ExecuteAll();

    auto results = bm_runner.BenchmarkResults();

    // Succeed if there's nothing to compare
    auto success = true;
    if (argc >= 2) {
        auto prev_results = benchmark::util::BenchmarkResultsIO::Load(argv[1]);
        BenchmarkComparer bm_comparer{std::move(prev_results)};
        success = bm_comparer.Compare(results);
    } else {
        std::cout << "Warning: no file with previous results provided\n";
    }

    if (argc >= 3) {
        benchmark::util::BenchmarkResultsIO::Save(results, argv[2]);
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
