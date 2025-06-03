#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

#include <easylogging++.h>

#include "adc_benchmark.h"
#include "benchmark_controller.h"
#include "benchmark_results.h"
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

    auto& controller = BenchmarkController::Instance();

    std::unordered_map<std::string, long long> prev_results;
    if (argc >= 2) {
        prev_results = benchmark::util::BenchmarkResults::Load(argv[1]);
    } else {
        std::cout << "Warning: no file with previous results provided\n";
    }
    controller.LoadPreviousResults(std::move(prev_results));

    for (auto test_register_func :
         {ADCBenchmark, DDBenchmark, INDBenchmark, FDBenchmark, MDBenchmark, NARBenchmark}) {
        test_register_func();
    }

    auto success = controller.RunAllTests();

    auto results = controller.GetResults();
    if (argc >= 3) {
        benchmark::util::BenchmarkResults::Save(results, argv[2]);
    } else {
        std::cout << "Total: " << results.size() << " tests:\n";
        for (auto const& [name, time] : results) {
            std::cout << '\t' << name << ": " << time << "ms\n";
        }
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
