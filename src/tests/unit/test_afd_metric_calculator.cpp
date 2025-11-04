#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/fd/afd_metric/afd_metric.h"
#include "algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "all_csv_configs.h"
#include "config/indices/type.h"

namespace tests {
using namespace config::names;
using namespace algos::afd_metric_calculator;

struct AFDMetricCalculatorParams {
    algos::StdParamsMap params;
    long double const expected = 0.L;

    AFDMetricCalculatorParams(config::IndicesType lhs_indices, config::IndicesType rhs_indices,
                              AFDMetric metric, long double expected,
                              CSVConfig const& csv_config = kTestFD)
        : params({{kCsvConfig, csv_config},
                  {kLhsIndices, std::move(lhs_indices)},
                  {kRhsIndices, std::move(rhs_indices)},
                  {kEqualNulls, true},
                  {kMetric, metric}}),
          expected(expected) {}
};

class TestAFDMetrics : public ::testing::TestWithParam<AFDMetricCalculatorParams> {};

TEST_P(TestAFDMetrics, DefaultTest) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto calculator =
            algos::CreateAndLoadAlgorithm<algos::afd_metric_calculator::AFDMetricCalculator>(mp);
    calculator->Execute();
    EXPECT_DOUBLE_EQ(calculator->GetResult(), p.expected);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        AFDMetricCalculatorTestSuite, TestAFDMetrics,
        ::testing::Values(
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::tau, 78.L/90),
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::g2, 1.L/6),
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::fi, 1 - std::log(4) / std::log(746496)),
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::mu_plus, 498.L/630),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::tau, 54.L/114),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::g2, 5.L/6),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::fi, std::log(432) / std::log(13824)),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::mu_plus, 252.L/912)
            ));
// clang-format on

}  // namespace tests
