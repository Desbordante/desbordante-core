#pragma once

#include <cmath>

#include "algorithms/nar/des/des.h"
#include "all_csv_configs.h"
#include "config/names.h"
#include "performance_testing.h"

namespace perf_tests {

inline void NARPerfTests() {
    using namespace config::names;

    PerformanceTesting::Instance().RegisterSimpleTest<algos::des::DES>(
            tests::kAdult, {{kMinimumSupport, 0.0},
                            {kMinimumConfidence, 0.0},
                            {kPopulationSize, static_cast<unsigned>(4 * std::pow(10, 4))},
                            {kMaxFitnessEvaluations, static_cast<unsigned>(4 * std::pow(10, 4))},
                            {kCrossoverProbability, 0.9},
                            {kDifferentialScale, 0.5},
                            {kDifferentialStrategy, +algos::des::DifferentialStrategy::rand1Bin}});
}

}  // namespace perf_tests
