#include <cstdlib>
#include <iostream>
#include <unordered_map>

#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <easylogging++.h>

#include "adc_benchmark.h"
#include "benchmark_cli.h"
#include "benchmark_comparer.h"
#include "benchmark_results_io.h"
#include "benchmark_runner.h"
#include "dd_benchmark.h"
#include "fd_benchmark.h"
#include "ind_benchmark.h"
#include "md_benchmark.h"
#include "nar_benchmark.h"

INITIALIZE_EASYLOGGINGPP

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    using namespace benchmark;

    el::Loggers::configureFromGlobal("logging.conf");

    BenchmarkRunner bm_runner;
    BenchmarkComparer bm_comparer;
    for (auto test_register_func :
         {ADCBenchmark, DDBenchmark, INDBenchmark, FDBenchmark, MDBenchmark, NARBenchmark}) {
        test_register_func(bm_runner, bm_comparer);
    }
    bm_runner.ExecuteAll();

    auto results = bm_runner.BenchmarkResults();

    BenchmarkCLI benchmark_cli;
    try {
        benchmark_cli.ParseOptions(argc, argv);
    } catch (boost::program_options::error const& e) {
        std::cout << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    auto const& var_map = benchmark_cli.GetVariablesMap();
    if (var_map.contains(kHelpLongOption)) {
        benchmark_cli.PrintHelp();
        return EXIT_SUCCESS;
    }

    // Succeed if there's nothing to compare
    auto success = true;
    if (var_map.contains(kBaselineLongOption)) {
        auto prev_results = benchmark::util::BenchmarkResultsIO::Load(
                var_map[kBaselineLongOption].as<std::string>());
        success = bm_comparer.Compare(prev_results, results);
    } else {
        std::cout << "Warning: no file with previous results provided\n";
    }

    if (var_map.contains(kOutputLongOption)) {
        benchmark::util::BenchmarkResultsIO::Save(results,
                                                  var_map[kOutputLongOption].as<std::string>());
    } else {
        std::cout << "Warning: no output filename provided\n";
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
