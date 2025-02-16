#pragma once

#include "algorithms/dc/FastADC/fastadc.h"
#include "all_csv_configs.h"
#include "performance_testing.h"

namespace perf_tests {

inline void ADCPerfTests() {
    PerformanceTesting::Instance().RegisterSimpleTest<algos::dc::FastADC>(tests::kNeighbors100k);
}

}  // namespace perf_tests
