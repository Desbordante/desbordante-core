#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

#include <easylogging++.h>

#include "adc_performance_tests.h"
#include "dd_performance_tests.h"
#include "fd_performance_tests.h"
#include "ind_performance_tests.h"
#include "md_performance_tests.h"
#include "nar_performance_tests.h"
#include "perf_tests_results.h"
#include "performance_testing.h"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char* argv[]) {
    using namespace perf_tests;

    el::Loggers::configureFromGlobal("logging.conf");

    PerformanceTesting& test_runner = PerformanceTesting::Instance();

    std::unordered_map<std::string, long long> prev_results;
    if (argc >= 2) {
        prev_results = perf_tests::util::PerformanceTestsResults::Load(argv[1]);
    } else {
        std::cout << "Warning: no file with previous results provided\n";
    }
    test_runner.LoadPreviousResults(std::move(prev_results));

    for (auto test_register_func :
         {ADCPerfTests, DDPerfTests, FDPerfTests, MDPerfTests, NARPerfTests, INDPerfTests}) {
        test_register_func();
    }

    auto success = test_runner.RunAllTests();

    auto results = test_runner.GetResults();
    if (argc >= 3) {
        perf_tests::util::PerformanceTestsResults::Save(results, argv[2]);
    } else {
        std::cout << "Total: " << results.size() << " tests:\n";
        for (auto const& [name, time] : results) {
            std::cout << '\t' << name << ": " << time << "ms\n";
        }
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
