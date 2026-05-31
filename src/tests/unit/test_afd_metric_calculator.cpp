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
                              AFDMetric metric, long double expected, bool use_pliws = false,
                              CSVConfig const& csv_config = kTestFD)
        : params({{kCsvConfig, csv_config},
                  {kLhsIndices, std::move(lhs_indices)},
                  {kRhsIndices, std::move(rhs_indices)},
                  {kEqualNulls, true},
                  {kUsePliws, use_pliws},
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
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::kTau, 78.L/90),
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::kTau, 78.L/90, true),
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG2, 1.L/6),
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG2, 1.L/6, true),
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::kFi, 1 - std::log(4) / std::log(746496)),
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::kFi, 1 - std::log(4) / std::log(746496), true),
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::kMuPlus, 498.L/630),
            AFDMetricCalculatorParams({4}, {3}, AFDMetric::kMuPlus, 498.L/630, true),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::kTau, 54.L/114),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::kTau, 54.L/114, true),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG2, 5.L/6),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG2, 5.L/6, true),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::kFi, std::log(432) / std::log(13824)),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::kFi, std::log(432) / std::log(13824), true),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::kMuPlus, 252.L/912),
            AFDMetricCalculatorParams({3}, {4}, AFDMetric::kMuPlus, 252.L/912, true)
            ));
// clang-format on

}  // namespace tests
