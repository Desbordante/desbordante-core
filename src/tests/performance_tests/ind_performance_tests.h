#pragma once

#include <cmath>

#include "algorithms/ind/faida/faida.h"
#include "algorithms/ind/mind/mind.h"
#include "all_csv_configs.h"
#include "config/error/type.h"
#include "config/names.h"
#include "config/thread_number/type.h"
#include "csv_config_util.h"
#include "performance_testing.h"

namespace perf_tests {

inline void INDPerfTests() {
    using namespace config::names;

    auto& testing = PerformanceTesting::Instance();

    // FAIDA tests are currently disabled, since test below runs 10 seconds, and for reasonable
    // run times we need really huge dataset
    // TODO(senichenkov): implement FAIDA tests
#if 0
    auto faida_test = [] {
        algos::StdParamsMap params{
                {kCsvConfigs,
                 tests::CSVConfigs{tests::kIowa1kk, tests::kMushroom, tests::kBreastCancer}},
                // Description says "higher sample size -- higher memory
                // consumption", but I don't know how it affects time
                {kSampleSize, 500},
                // Closer to 0 -- higher memory consumption and slower algo
                // Not every value is accepted
                {kHllAccuracy, std::pow(10, -4)},
                {kThreads, static_cast<config::ThreadNumType>(1)}};
        auto algo = algos::CreateAndLoadAlgorithm<algos::Faida>(params);
        algo->Execute();
    };
    testing.RegisterTest(std::move(faida_test), "Faida, Iowa1kk, mushroom, breast_cancer", 20);
#endif

    auto mind_test = [] {
        algos::StdParamsMap params{{kCsvConfigs, tests::CSVConfigs{tests::kIowa450k}},
                                   {kError, static_cast<config::ErrorType>(0.8)}};
        auto algo = algos::CreateAndLoadAlgorithm<algos::Mind>(params);
        algo->Execute();
    };
    testing.RegisterTest(std::move(mind_test), "Mind, Iowa450k");
}

}  // namespace perf_tests
