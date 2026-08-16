#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/afd_metric/afd_metric.h"
#include "core/algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

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

INSTANTIATE_TEST_SUITE_P(
        AFDMetricCalculatorTestSuite, TestAFDMetrics,
        ::testing::Values(
                AFDMetricCalculatorParams({4}, {3}, AFDMetric::kTau, 78.L / 90),
                AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG2, 1.L / 6),
                AFDMetricCalculatorParams({4}, {3}, AFDMetric::kFi,
                                          1 - std::log(4) / std::log(746496)),
                AFDMetricCalculatorParams({4}, {3}, AFDMetric::kMuPlus, 498.L / 630),
                AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG3, 11.L / 12),
                AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG1, 1.L / 12),
                AFDMetricCalculatorParams({4}, {3}, AFDMetric::kRho, 0.8333333333333334),
                AFDMetricCalculatorParams({3}, {4}, AFDMetric::kTau, 54.L / 114),
                AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG2, 5.L / 6),
                AFDMetricCalculatorParams({3}, {4}, AFDMetric::kFi,
                                          std::log(432) / std::log(13824)),
                AFDMetricCalculatorParams({3}, {4}, AFDMetric::kMuPlus, 252.L / 912),
                AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG3, 7.L / 12),
                AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG1, 13.L / 12),
                AFDMetricCalculatorParams({3}, {4}, AFDMetric::kRho, 0.6666666666666666),
                AFDMetricCalculatorParams({2}, {3}, AFDMetric::kPerValue, 0.9375),
                AFDMetricCalculatorParams({4}, {5}, AFDMetric::kPerValue, 0.6666666666666666),
                AFDMetricCalculatorParams({3}, {2}, AFDMetric::kPerValue, 0.708333333333333),
                AFDMetricCalculatorParams({0}, {1}, AFDMetric::kPerValue, 0.25),
                AFDMetricCalculatorParams({1}, {0}, AFDMetric::kPerValue, 1.0),
                AFDMetricCalculatorParams({4}, {3}, AFDMetric::kPerValue, 0.9),
                AFDMetricCalculatorParams({1}, {5}, AFDMetric::kPerValue, 0.583333333333333),
                AFDMetricCalculatorParams({5}, {1}, AFDMetric::kPerValue, 1.0),
                AFDMetricCalculatorParams({2}, {3}, AFDMetric::kG3, 0.9166666666666666),
                AFDMetricCalculatorParams({4}, {5}, AFDMetric::kG3, 0.6666666666666666),
                AFDMetricCalculatorParams({3}, {2}, AFDMetric::kG3, 0.5),
                AFDMetricCalculatorParams({0}, {1}, AFDMetric::kG3, 0.25),
                AFDMetricCalculatorParams({1}, {0}, AFDMetric::kG3, 1.0),
                AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG3, 0.9166666666666666),
                AFDMetricCalculatorParams({1}, {5}, AFDMetric::kG3, 0.583333333333333),
                AFDMetricCalculatorParams({5}, {1}, AFDMetric::kG3, 1.0)));

}  // namespace tests
