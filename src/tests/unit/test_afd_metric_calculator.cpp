#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "core/util/enum_to_str.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {
using namespace config::names;
using namespace algos::afd_metric_calculator;

namespace {
constexpr long double kTolerance = 1e-12L;
CSVConfig const kRfiPlus{kTestDataDir / "RfiPlus.csv", ',', true};
long double const kBinaryEntropy = std::log(3.L) - 2.L / 3.L * std::log(2.L);

long double CalculateScore(config::IndicesType lhs, config::IndicesType rhs, AFDMetric metric,
                           CSVConfig const& csv = kTestFD) {
    algos::StdParamsMap params{{kCsvConfig, csv},
                               {kLhsIndices, std::move(lhs)},
                               {kRhsIndices, std::move(rhs)},
                               {kEqualNulls, true},
                               {kMetric, metric}};
    auto calculator = algos::CreateAndLoadAlgorithm<AFDMetricCalculator>(params);
    calculator->Execute();
    return calculator->GetResult();
}
}  // namespace

// ---- Expected scores ----

struct MetricCase {
    char const* name;
    config::IndicesType lhs;
    config::IndicesType rhs;
    AFDMetric metric;
    long double expected;
    CSVConfig csv = kTestFD;
};

class TestAFDMetrics : public ::testing::TestWithParam<MetricCase> {};

TEST_P(TestAFDMetrics, CalculatesExpectedScore) {
    auto const& p = GetParam();
    EXPECT_NEAR(CalculateScore(p.lhs, p.rhs, p.metric, p.csv), p.expected, kTolerance);
}

INSTANTIATE_TEST_SUITE_P(
        AFDMetricCalculatorTestSuite, TestAFDMetrics,
        ::testing::Values(
                MetricCase{"Forward", {4}, {3}, AFDMetric::kTau, 78.L / 90},
                MetricCase{"Forward", {4}, {3}, AFDMetric::kG2, 5.L / 6},
                MetricCase{"Forward", {4}, {3}, AFDMetric::kFi, 1 - std::log(4) / std::log(746496)},
                MetricCase{"Forward", {4}, {3}, AFDMetric::kMuPlus, 498.L / 630},
                MetricCase{"Forward", {4}, {3}, AFDMetric::kG3, 11.L / 12},
                MetricCase{"Forward", {4}, {3}, AFDMetric::kG1Prime, 62.L / 63},
                MetricCase{"Forward", {4}, {3}, AFDMetric::kG3Prime, 6.L / 7},
                MetricCase{"Forward", {4}, {3}, AFDMetric::kG1, 65.L / 66},
                MetricCase{"Forward", {4}, {3}, AFDMetric::kRho, 5.L / 6},
                MetricCase{"Forward", {4}, {3}, AFDMetric::kPdep, 11.L / 12},
                MetricCase{"Reverse", {3}, {4}, AFDMetric::kTau, 54.L / 114},
                MetricCase{"Reverse", {3}, {4}, AFDMetric::kG2, 1.L / 6},
                MetricCase{"Reverse", {3}, {4}, AFDMetric::kFi, std::log(432) / std::log(13824)},
                MetricCase{"Reverse", {3}, {4}, AFDMetric::kMuPlus, 252.L / 912},
                MetricCase{"Reverse", {3}, {4}, AFDMetric::kG3, 7.L / 12},
                MetricCase{"Reverse", {3}, {4}, AFDMetric::kG1Prime, 50.L / 63},
                MetricCase{"Reverse", {3}, {4}, AFDMetric::kG3Prime, 3.L / 8},
                MetricCase{"Reverse", {3}, {4}, AFDMetric::kG1, 53.L / 66},
                MetricCase{"Reverse", {3}, {4}, AFDMetric::kRho, 2.L / 3},
                MetricCase{"Reverse", {3}, {4}, AFDMetric::kPdep, 7.L / 12},
                // Observed FI is 1/2 and its chance correction is 1/5.
                MetricCase{"Nonexact", {0}, {1}, AFDMetric::kRfiPlus, 0.3L, kRfiPlus},
                MetricCase{"Independent", {2}, {3}, AFDMetric::kRfiPlus, 0.L, kRfiPlus},
                MetricCase{"Nonexact", {0}, {1}, AFDMetric::kRfiPrimePlus, 0.375L, kRfiPlus},
                MetricCase{"Independent", {2}, {3}, AFDMetric::kRfiPrimePlus, 0.L, kRfiPlus},
                // SFI adds 1/2 to each cell: {{3, 0}, {1, 2}} becomes {{3.5, 0.5}, {1.5, 2.5}}.
                MetricCase{
                        "HalfSmoothing", {0}, {1}, AFDMetric::kSfi, 0.21524251989462606L, kRfiPlus},
                MetricCase{"Independent", {2}, {3}, AFDMetric::kSfi, 0.L, kRfiPlus},
                MetricCase{"Nonexact",
                           {0},
                           {1},
                           AFDMetric::kG1S,
                           1.L - kBinaryEntropy / 2.L,
                           kRfiPlus},
                MetricCase{
                        "Independent", {2}, {3}, AFDMetric::kG1S, 1.L - kBinaryEntropy, kRfiPlus},
                MetricCase{"ClampedToZero", {7}, {6}, AFDMetric::kG1S, 0.L, kRfiPlus}),
        [](auto const& info) {
            return std::string{util::EnumToStr(info.param.metric)} + "_" + info.param.name;
        });

// ---- Exact dependencies and one-row input ----

struct CalculatorExactDependencyParams {
    AFDMetric metric;
    long double constant_rhs_score;
    long double one_row_score;
};

class TestCalculatorExactDependencies
    : public ::testing::TestWithParam<CalculatorExactDependencyParams> {};

TEST_P(TestCalculatorExactDependencies, RepeatedLhs) {
    EXPECT_EQ(CalculateScore({5}, {1}, GetParam().metric), 1.L);
}

TEST_P(TestCalculatorExactDependencies, UniqueLhs) {
    EXPECT_EQ(CalculateScore({3}, {4}, GetParam().metric, kSimpleTypes), 1.L);
}

TEST_P(TestCalculatorExactDependencies, ConstantRhs) {
    auto const& p = GetParam();
    EXPECT_EQ(CalculateScore({4}, {0}, p.metric), p.constant_rhs_score);
}

TEST_P(TestCalculatorExactDependencies, OneRow) {
    auto const& p = GetParam();
    CSVConfig const one_row{kTestDataDir / "OneRow.csv", ',', true};
    EXPECT_EQ(CalculateScore({0}, {1}, p.metric, one_row), p.one_row_score);
}

// FI defines a constant RHS score as zero, including on a one-row table.
// G1 has no row pairs on one row: its infinite error produces a negative-infinite score.
INSTANTIATE_TEST_SUITE_P(
        AllMeasures, TestCalculatorExactDependencies,
        ::testing::Values(
                CalculatorExactDependencyParams{AFDMetric::kG1, 1.L,
                                                -std::numeric_limits<long double>::infinity()},
                CalculatorExactDependencyParams{AFDMetric::kPdep, 1.L, 1.L},
                CalculatorExactDependencyParams{AFDMetric::kTau, 1.L, 1.L},
                CalculatorExactDependencyParams{AFDMetric::kMuPlus, 1.L, 1.L},
                CalculatorExactDependencyParams{AFDMetric::kRho, 1.L, 1.L},
                CalculatorExactDependencyParams{AFDMetric::kFi, 0.L, 0.L},
                CalculatorExactDependencyParams{AFDMetric::kG2, 1.L, 1.L},
                CalculatorExactDependencyParams{AFDMetric::kG3, 1.L, 1.L},
                CalculatorExactDependencyParams{AFDMetric::kG1S, 1.L, 1.L},
                CalculatorExactDependencyParams{AFDMetric::kRfiPlus, 1.L, 1.L},
                CalculatorExactDependencyParams{AFDMetric::kRfiPrimePlus, 1.L, 1.L},
                CalculatorExactDependencyParams{AFDMetric::kSfi, 1.L, 1.L},
                CalculatorExactDependencyParams{AFDMetric::kG1Prime, 1.L, 1.L},
                CalculatorExactDependencyParams{AFDMetric::kG3Prime, 1.L, 1.L}),
        [](auto const& info) { return std::string{util::EnumToStr(info.param.metric)}; });

// ---- G1-prime complete-row normalization ----

struct G1PrimeNormalizationParams {
    char const* name;
    char const* file;
    unsigned long long normalization;
    long double expected_error;
};

class TestG1PrimeNormalization : public ::testing::TestWithParam<G1PrimeNormalizationParams> {};

TEST_P(TestG1PrimeNormalization, CalculatesScoreUsingCompleteRows) {
    auto const& p = GetParam();
    CSVConfig const csv{kTestDataDir / p.file, ',', true};
    EXPECT_NEAR(CalculateScore({0}, {1}, AFDMetric::kG1Prime, csv), 1.L - p.expected_error,
                kTolerance);
}

TEST_P(TestG1PrimeNormalization, CalculatesOrdinaryAndEmptyLhsErrors) {
    auto const& p = GetParam();
    auto table = MakeInputTable(CSVConfig{kTestDataDir / p.file, ',', true});
    auto relation = ColumnLayoutRelationData::CreateFrom(*table);
    auto lhs = relation->CalculatePLI({0});
    auto joint = relation->CalculatePLI({0, 1});
    auto const* rhs = &relation->GetColumnData(1);

    EXPECT_NEAR(AFDMetricCalculator::CalculateG1PrimeError(lhs.get(), joint.get(), p.normalization),
                p.expected_error, kTolerance);
    EXPECT_NEAR(AFDMetricCalculator::CalculateZeroAryG1Prime(rhs, *relation), p.expected_error,
                kTolerance);
    EXPECT_NEAR(AFDMetricCalculator::CalculateZeroAryG1Prime(rhs, relation->GetNumTuplePairs(),
                                                             p.normalization),
                p.expected_error, kTolerance);
}

// Both inputs have two violating pairs. Only the first has a complete-row duplicate,
// so the denominators are 3 - 1 = 2 and 3 - 0 = 3.
INSTANTIATE_TEST_SUITE_P(
        CompleteRowNormalization, TestG1PrimeNormalization,
        ::testing::Values(G1PrimeNormalizationParams{"CompleteRowDuplicates",
                                                     "G1PrimeDuplicates.csv", 2, 1.L},
                          G1PrimeNormalizationParams{"MatchingProjectionOnly",
                                                     "G1PrimeProjection.csv", 3, 2.L / 3.L}),
        [](auto const& info) { return info.param.name; });

// ---- G1-prime zero-denominator handling ----

struct G1PrimeExactDependencyParams {
    char const* name;
    char const* file;
    model::ColumnIndex lhs;
    model::ColumnIndex rhs;
};

class TestG1PrimeZeroNormalization : public ::testing::TestWithParam<G1PrimeExactDependencyParams> {
};

TEST_P(TestG1PrimeZeroNormalization, ExactDependenciesHaveZeroError) {
    auto const& p = GetParam();
    auto table = MakeInputTable(CSVConfig{kTestDataDir / p.file, ',', true});
    auto relation = ColumnLayoutRelationData::CreateFrom(*table);
    auto lhs = relation->CalculatePLI({p.lhs});
    auto joint = relation->CalculatePLI({p.lhs, p.rhs});
    auto const* rhs = &relation->GetColumnData(p.rhs);

    EXPECT_EQ(AFDMetricCalculator::CalculateG1PrimeError(lhs.get(), joint.get(), *relation), 0);
    EXPECT_EQ(AFDMetricCalculator::CalculateZeroAryG1Prime(rhs, *relation), 0);
    // An exact dependency has zero error even when the caller supplies denominator zero.
    EXPECT_EQ(AFDMetricCalculator::CalculateG1PrimeError(lhs.get(), joint.get(), 0ULL), 0);
    EXPECT_EQ(AFDMetricCalculator::CalculateZeroAryG1Prime(rhs, relation->GetNumTuplePairs(), 0),
              0);
}

INSTANTIATE_TEST_SUITE_P(
        ZeroNormalization, TestG1PrimeZeroNormalization,
        ::testing::Values(G1PrimeExactDependencyParams{"OneRow", "OneRow.csv", 0, 1},
                          G1PrimeExactDependencyParams{"IdenticalRows", "G1PrimeIdenticalRows.csv",
                                                       0, 1},
                          G1PrimeExactDependencyParams{"ConstantRhs", "TestFD.csv", 4, 0}),
        [](auto const& info) { return info.param.name; });

TEST(AFDMetricCalculatorTest, G1PrimeRejectsZeroNormalizationForViolations) {
    auto table = MakeInputTable(CSVConfig{kTestDataDir / "G1PrimeDuplicates.csv", ',', true});
    auto relation = ColumnLayoutRelationData::CreateFrom(*table);
    auto lhs = relation->CalculatePLI({0});
    auto joint = relation->CalculatePLI({0, 1});
    EXPECT_THROW(AFDMetricCalculator::CalculateG1PrimeError(lhs.get(), joint.get(), 0ULL),
                 std::invalid_argument);
    EXPECT_THROW(AFDMetricCalculator::CalculateZeroAryG1Prime(&relation->GetColumnData(1),
                                                              relation->GetNumTuplePairs(), 0),
                 std::invalid_argument);
}

}  // namespace tests
