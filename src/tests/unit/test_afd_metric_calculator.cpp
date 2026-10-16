#include <utility>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/afd_metric/afd_metric.h"
#include "core/algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {
using namespace config::names;
using namespace algos::afd_metric_calculator;

namespace {

CSVConfig const kRfiPlus{kTestDataDir / "RfiPlus.csv", ',', true};
CSVConfig const kRfiPlusOneRow{kTestDataDir / "RfiPlusOneRow.csv", ',', true};
constexpr long double kTolerance = 1e-12L;

long double CalculateMetric(config::IndicesType lhs_indices, config::IndicesType rhs_indices,
                            AFDMetric metric, CSVConfig const& csv_config) {
    algos::StdParamsMap params{{kCsvConfig, csv_config},
                               {kLhsIndices, std::move(lhs_indices)},
                               {kRhsIndices, std::move(rhs_indices)},
                               {kEqualNulls, true},
                               {kMetric, metric}};
    auto calculator =
            algos::CreateAndLoadAlgorithm<algos::afd_metric_calculator::AFDMetricCalculator>(
                    params);
    calculator->Execute();
    return calculator->GetResult();
}

}  // namespace

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
    EXPECT_NEAR(calculator->GetResult(), p.expected, kTolerance);
}

INSTANTIATE_TEST_SUITE_P(
        AFDMetricCalculatorTestSuite, TestAFDMetrics,
        ::testing::Values(AFDMetricCalculatorParams({4}, {3}, AFDMetric::kTau, 78.L / 90),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG2, 5.L / 6),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kFi,
                                                    1 - std::log(4) / std::log(746496)),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kMuPlus, 498.L / 630),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG3, 11.L / 12),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kG1, 65.L / 66),
                          AFDMetricCalculatorParams({4}, {3}, AFDMetric::kRho, 0.8333333333333334),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kTau, 54.L / 114),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG2, 1.L / 6),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kFi,
                                                    std::log(432) / std::log(13824)),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kMuPlus, 252.L / 912),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG3, 7.L / 12),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kG1, 53.L / 66),
                          AFDMetricCalculatorParams({3}, {4}, AFDMetric::kRho,
                                                    0.6666666666666666)));

TEST(AFDMetricCalculatorTest, CalculatesRfiPlus) {
    EXPECT_NEAR(CalculateMetric({0}, {1}, AFDMetric::kRfiPlus, kRfiPlus), 0.3L, kTolerance);
    EXPECT_NEAR(CalculateMetric({2}, {3}, AFDMetric::kRfiPlus, kRfiPlus), 0.L, kTolerance);
}

TEST(AFDMetricCalculatorTest, CalculatesRfiPrimePlus) {
    EXPECT_NEAR(CalculateMetric({0}, {1}, AFDMetric::kRfiPrimePlus, kRfiPlus), 0.375L, kTolerance);
    EXPECT_NEAR(CalculateMetric({2}, {3}, AFDMetric::kRfiPrimePlus, kRfiPlus), 0.L, kTolerance);
}

TEST(AFDMetricCalculatorTest, CalculatesShannonG1) {
    long double const binary_entropy = std::log(3.L) - 2.L / 3.L * std::log(2.L);
    EXPECT_NEAR(CalculateMetric({0}, {1}, AFDMetric::kG1S, kRfiPlus), 1.L - binary_entropy / 2.L,
                kTolerance);
    EXPECT_NEAR(CalculateMetric({2}, {3}, AFDMetric::kG1S, kRfiPlus), 1.L - binary_entropy,
                kTolerance);
    EXPECT_NEAR(CalculateMetric({7}, {6}, AFDMetric::kG1S, kRfiPlus), 0.L, kTolerance);
}

TEST(AFDMetricCalculatorTest, ReliableMetricsReturnOneForExactDependencies) {
    for (AFDMetric metric : {AFDMetric::kRfiPlus, AFDMetric::kRfiPrimePlus}) {
        SCOPED_TRACE(static_cast<int>(metric));
        EXPECT_NEAR(CalculateMetric({4}, {5}, metric, kRfiPlus), 1.L, kTolerance);
        EXPECT_NEAR(CalculateMetric({6}, {1}, metric, kRfiPlus), 1.L, kTolerance);
        EXPECT_NEAR(CalculateMetric({0}, {7}, metric, kRfiPlus), 1.L, kTolerance);
        EXPECT_NEAR(CalculateMetric({0}, {1}, metric, kRfiPlusOneRow), 1.L, kTolerance);
    }
}

TEST(AFDMetricCalculatorTest, ShannonG1ReturnsOneForExactDependencies) {
    EXPECT_NEAR(CalculateMetric({4}, {5}, AFDMetric::kG1S, kRfiPlus), 1.L, kTolerance);
    EXPECT_NEAR(CalculateMetric({6}, {1}, AFDMetric::kG1S, kRfiPlus), 1.L, kTolerance);
    EXPECT_NEAR(CalculateMetric({0}, {7}, AFDMetric::kG1S, kRfiPlus), 1.L, kTolerance);
    EXPECT_NEAR(CalculateMetric({0}, {1}, AFDMetric::kG1S, kRfiPlusOneRow), 1.L, kTolerance);
}

}  // namespace tests
