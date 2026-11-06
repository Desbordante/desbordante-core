#include <cmath>
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
    auto calculator = algos::CreateAndLoadAlgorithm<AFDMetricCalculator>(params);
    calculator->Execute();
    return calculator->GetResult();
}

}  // namespace

TEST(AFDMetricCalculatorTest, CalculatesRfiPlus) {
    EXPECT_NEAR(CalculateMetric({0}, {1}, AFDMetric::kRfiPlus, kRfiPlus), 0.3L, kTolerance);
    EXPECT_NEAR(CalculateMetric({2}, {3}, AFDMetric::kRfiPlus, kRfiPlus), 0.L, kTolerance);
}

TEST(AFDMetricCalculatorTest, G1PrimeNormalizesCompleteRowsRatherThanProjection) {
    // Both projections have two violating pairs. Only the first relation has a
    // complete-row duplicate: denominators are 3 - 1 = 2 and 3 - 0 = 3.
    CSVConfig const duplicates{kTestDataDir / "G1PrimeDuplicates.csv", ',', true};
    CSVConfig const projection{kTestDataDir / "G1PrimeProjection.csv", ',', true};
    // Alternate distinct relations across repeated static API calls to catch stale
    // normalization retained in cross-call/global state, even with fresh instances.
    for (int pass = 0; pass < 2; ++pass) {
        SCOPED_TRACE(::testing::Message() << "pass=" << pass);
        for (bool full_duplicates : {true, false}) {
            SCOPED_TRACE(::testing::Message()
                         << "input=" << (full_duplicates ? "duplicates" : "projection"));
            auto const& csv = full_duplicates ? duplicates : projection;
            auto table = MakeInputTable(csv);
            auto relation = ColumnLayoutRelationData::CreateFrom(*table);
            auto lhs = relation->CalculatePLI({0});
            auto joint = relation->CalculatePLI({0, 1});
            auto const* rhs = &relation->GetColumnData(1);
            unsigned long long const normalization = full_duplicates ? 2 : 3;
            double const error = full_duplicates ? 1.0 : 2.0 / 3.0;
            EXPECT_NEAR(CalculateMetric({0}, {1}, AFDMetric::kG1Prime, csv), 1.0 - error,
                        kTolerance);
            // The last, constant column models an empty LHS without changing duplicates.
            config::IndicesType const constant{
                    static_cast<model::ColumnIndex>(table->GetNumberOfColumns() - 1)};
            EXPECT_NEAR(CalculateMetric(constant, {1}, AFDMetric::kG1Prime, csv), 1.0 - error,
                        kTolerance);
            EXPECT_NEAR(
                    AFDMetricCalculator::CalculateG1PrimeError(lhs.get(), joint.get(), *relation),
                    error, kTolerance);
            EXPECT_NEAR(AFDMetricCalculator::CalculateG1PrimeError(lhs.get(), joint.get(),
                                                                   normalization),
                        error, kTolerance);
            EXPECT_NEAR(AFDMetricCalculator::CalculateZeroAryG1Prime(rhs, *relation), error,
                        kTolerance);
            EXPECT_NEAR(AFDMetricCalculator::CalculateZeroAryG1Prime(rhs, 3, normalization), error,
                        kTolerance);
        }
    }
}

TEST(AFDMetricCalculatorTest, G1PrimeExactAndInvalidNormalizationConventions) {
    for (auto const* file :
         {"G1PrimeOneRow.csv", "G1PrimeIdenticalRows.csv", "G1PrimeConstantRhs.csv"}) {
        SCOPED_TRACE(file);
        CSVConfig const csv{kTestDataDir / file, ',', true};
        auto table = MakeInputTable(csv);
        auto relation = ColumnLayoutRelationData::CreateFrom(*table);
        auto lhs = relation->CalculatePLI({0});
        auto joint = relation->CalculatePLI({0, 1});
        auto const* rhs = &relation->GetColumnData(1);
        EXPECT_EQ(AFDMetricCalculator::CalculateG1PrimeError(lhs.get(), joint.get(), *relation), 0);
        EXPECT_EQ(AFDMetricCalculator::CalculateG1PrimeError(lhs.get(), joint.get(), 0ULL), 0);
        EXPECT_EQ(AFDMetricCalculator::CalculateZeroAryG1Prime(rhs, *relation), 0);
        EXPECT_EQ(
                AFDMetricCalculator::CalculateZeroAryG1Prime(rhs, relation->GetNumTuplePairs(), 0),
                0);
        EXPECT_EQ(CalculateMetric({0}, {1}, AFDMetric::kG1Prime, csv), 1);
        EXPECT_EQ(CalculateMetric({2}, {1}, AFDMetric::kG1Prime, csv), 1);
    }
    auto table = MakeInputTable(CSVConfig{kTestDataDir / "G1PrimeViolation.csv", ',', true});
    auto relation = ColumnLayoutRelationData::CreateFrom(*table);
    auto lhs = relation->CalculatePLI({0});
    auto joint = relation->CalculatePLI({0, 1});
    EXPECT_THROW(AFDMetricCalculator::CalculateG1PrimeError(lhs.get(), joint.get(), 0ULL),
                 std::invalid_argument);
    EXPECT_THROW(AFDMetricCalculator::CalculateZeroAryG1Prime(&relation->GetColumnData(1), 1, 0),
                 std::invalid_argument);
}

TEST(AFDMetricCalculatorTest, CalculatesRfiPrimePlus) {
    EXPECT_NEAR(CalculateMetric({0}, {1}, AFDMetric::kRfiPrimePlus, kRfiPlus), 0.375L, kTolerance);
    EXPECT_NEAR(CalculateMetric({2}, {3}, AFDMetric::kRfiPrimePlus, kRfiPlus), 0.L, kTolerance);
}

TEST(AFDMetricCalculatorTest, CalculatesSfiWithHalfSmoothing) {
    EXPECT_NEAR(CalculateMetric({0}, {1}, AFDMetric::kSfi, kRfiPlus), 0.21524251989462606L,
                kTolerance);
    EXPECT_NEAR(CalculateMetric({2}, {3}, AFDMetric::kSfi, kRfiPlus), 0.L, kTolerance);
}

TEST(AFDMetricCalculatorTest, CalculatesShannonG1) {
    long double const binary_entropy = std::log(3.L) - 2.L / 3.L * std::log(2.L);
    EXPECT_NEAR(CalculateMetric({0}, {1}, AFDMetric::kG1S, kRfiPlus), 1.L - binary_entropy / 2.L,
                kTolerance);
    EXPECT_NEAR(CalculateMetric({2}, {3}, AFDMetric::kG1S, kRfiPlus), 1.L - binary_entropy,
                kTolerance);
    EXPECT_NEAR(CalculateMetric({7}, {6}, AFDMetric::kG1S, kRfiPlus), 0.L, kTolerance);
}

TEST(AFDMetricCalculatorTest, NewMetricsReturnOneForExactDependencies) {
    for (AFDMetric metric : {AFDMetric::kRfiPlus, AFDMetric::kRfiPrimePlus, AFDMetric::kSfi,
                             AFDMetric::kG1Prime, AFDMetric::kG3Prime}) {
        SCOPED_TRACE(static_cast<int>(metric));
        EXPECT_NEAR(CalculateMetric({4}, {5}, metric, kRfiPlus), 1.L, kTolerance);
        EXPECT_NEAR(CalculateMetric({6}, {1}, metric, kRfiPlus), 1.L, kTolerance);
        EXPECT_NEAR(CalculateMetric({0}, {7}, metric, kRfiPlus), 1.L, kTolerance);
        EXPECT_NEAR(CalculateMetric({0}, {1}, metric, kRfiPlusOneRow), 1.L, kTolerance);
    }
}

TEST(AFDMetricCalculatorTest, NewMetricsAreEqualPartitionInvariant) {
    for (AFDMetric metric : {AFDMetric::kSfi, AFDMetric::kG1Prime, AFDMetric::kG3Prime}) {
        SCOPED_TRACE(static_cast<int>(metric));
        long double const score = CalculateMetric({4}, {1}, metric, kRfiPlus);
        EXPECT_NEAR(CalculateMetric({4, 5}, {1}, metric, kRfiPlus), score, kTolerance);
    }
}

TEST(AFDMetricCalculatorTest, ShannonG1ReturnsOneForExactDependencies) {
    EXPECT_NEAR(CalculateMetric({4}, {5}, AFDMetric::kG1S, kRfiPlus), 1.L, kTolerance);
    EXPECT_NEAR(CalculateMetric({6}, {1}, AFDMetric::kG1S, kRfiPlus), 1.L, kTolerance);
    EXPECT_NEAR(CalculateMetric({0}, {7}, AFDMetric::kG1S, kRfiPlus), 1.L, kTolerance);
    EXPECT_NEAR(CalculateMetric({0}, {1}, AFDMetric::kG1S, kRfiPlusOneRow), 1.L, kTolerance);
}

}  // namespace tests
