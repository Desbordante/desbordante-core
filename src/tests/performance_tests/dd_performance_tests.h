#pragma once

#include "algorithms/dd/split/split.h"
#include "all_csv_configs.h"
#include "config/names.h"
#include "performance_testing.h"

namespace perf_tests {

inline void DDPerfTests() {
    PerformanceTesting::Instance().RegisterSimpleTest<algos::dd::Split>(tests::kAdult5attr);
}

}  // namespace perf_tests
