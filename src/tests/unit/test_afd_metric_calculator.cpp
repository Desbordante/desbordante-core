#include <cmath>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {
using namespace config::names;
using namespace algos::afd_metric_calculator;

struct AFDMetricCalculatorParams {
    config::IndicesType lhs;
    config::IndicesType rhs;
    AFDMetric metric;
    long double expected;
};

class TestAFDMetrics : public ::testing::TestWithParam<AFDMetricCalculatorParams> {};

TEST_P(TestAFDMetrics, DefaultTest) {
    auto const& p = GetParam();
    algos::StdParamsMap params{{kCsvConfig, kTestFD},
                               {kLhsIndices, p.lhs},
                               {kRhsIndices, p.rhs},
                               {kEqualNulls, true},
                               {kMetric, p.metric}};
    auto calculator = algos::CreateAndLoadAlgorithm<AFDMetricCalculator>(params);
    calculator->Execute();
    EXPECT_NEAR(calculator->GetResult(), p.expected, 1e-12L);
}

INSTANTIATE_TEST_SUITE_P(
        AFDMetricCalculatorTestSuite, TestAFDMetrics,
        ::testing::Values(AFDMetricCalculatorParams({4}, {3}, AFDMetric::kTau, 78.L / 90),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG2, 5.L / 6),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kFi,
                                                    1 - std::log(4) / std::log(746496)),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kMuPlus, 498.L / 630),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG3, 11.L / 12),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG1Prime, 62.L / 63),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG3Prime, 6.L / 7),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG1, 65.L / 66),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kRho, 0.8333333333333334),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kTau, 54.L / 114),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG2, 1.L / 6),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kFi,
                                                    std::log(432) / std::log(13824)),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kMuPlus, 252.L / 912),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG3, 7.L / 12),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG1Prime, 50.L / 63),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG3Prime, 3.L / 8),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG1, 53.L / 66),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kRho,
                                                    0.6666666666666666)));

}  // namespace tests
