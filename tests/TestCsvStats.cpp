#include <gmock/gmock.h>
#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <memory>

#include "BuiltIn.h"
#include "CSVParser.h"
#include "ColumnLayoutTypedRelationData.h"
#include "CsvStats.h"
#include "NumericType.h"
#include "TypedColumnData.h"

namespace tests {

struct CsvStatsParams {
    std::string_view dataset_;
    char const sep_;
    bool const has_header_;

    CsvStatsParams(std::string_view dataset, char const sep = ',',
                   bool const has_header = true) noexcept
        : dataset_(dataset), sep_(sep), has_header_(has_header) {}
};

class TestCsvStats : public ::testing::TestWithParam<CsvStatsParams> {};

TEST_P(TestCsvStats, TestNullEmpties) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);
    EXPECT_EQ(0, stats.GetMin(0).has_value());
    EXPECT_EQ(0, stats.GetMax(0).has_value());
    EXPECT_EQ(0, stats.GetSum(0).has_value());
    EXPECT_EQ(0, stats.GetAvg(0).has_value());
    EXPECT_EQ(0, stats.GetSTD(0).has_value());
    EXPECT_EQ(0, stats.GetSkewness(0).has_value());
    EXPECT_EQ(0, stats.GetKurtosis(0).has_value());
    EXPECT_EQ(0, stats.GetQuantile(0.25, 0).has_value());
    EXPECT_EQ(0, stats.GetQuantile(0.5, 0).has_value());
    EXPECT_EQ(0, stats.GetQuantile(0.75, 0).has_value());
}

TEST_P(TestCsvStats, TestMinString) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);
    auto pair = stats.GetMin(1);
    mo::Type const& type = static_cast<mo::Type const&>(*pair->second);
    auto min = type.GetValue<mo::String>(pair->first);
    EXPECT_EQ("a", min);
}

TEST_P(TestCsvStats, TestMaxString) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);
    auto pair = stats.GetMax(1);
    mo::Type const& type = static_cast<mo::Type const&>(*pair->second);
    auto max = type.GetValue<mo::String>(pair->first);
    EXPECT_EQ("abd", max);
}

TEST_P(TestCsvStats, TestMinDouble) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);
    auto pair = stats.GetMin(2);
    mo::Type const& type = static_cast<mo::Type const&>(*pair->second);
    auto min = type.GetValue<mo::Double>(pair->first);
    EXPECT_DOUBLE_EQ(1.07, min);
}

TEST_P(TestCsvStats, TestMaxDouble) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);
    auto pair = stats.GetMax(2);
    mo::Type const& type = static_cast<mo::Type const&>(*pair->second);
    auto max = type.GetValue<mo::Double>(pair->first);
    EXPECT_DOUBLE_EQ(143.9, max);
}

TEST_P(TestCsvStats, TestSumDouble) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);
    auto pair = stats.GetSum(2);
    mo::INumericType const& type = static_cast<mo::INumericType const&>(*pair->second);
    std::unique_ptr<std::byte const[]> sum(pair->first);
    EXPECT_DOUBLE_EQ(212.61, type.GetValue<mo::Double>(sum.get()));
}
TEST_P(TestCsvStats, TestCount) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);
    int count = stats.Count(0);
    EXPECT_EQ(0, count);
}
TEST_P(TestCsvStats, TestDistinct) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);
    auto distinct = stats.Distinct(3);
    EXPECT_EQ(5, distinct);
}

TEST_P(TestCsvStats, TestIsCategorial) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);
    auto distinct = stats.isCategorial(3, 5);
    EXPECT_EQ(true, distinct);
}

TEST_P(TestCsvStats, TestGetQuantiles) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);

    auto quantile_0_25 = stats.GetQuantile(0.25, 4);
    auto result1 = quantile_0_25->second->GetValue<mo::Int>(quantile_0_25->first);
    auto quantile_0_5 = stats.GetQuantile(0.5, 4);
    auto result2 = quantile_0_5->second->GetValue<mo::Int>(quantile_0_5->first);
    auto quantile_0_75 = stats.GetQuantile(0.75, 4);
    auto result3 = quantile_0_75->second->GetValue<mo::Int>(quantile_0_75->first);

    EXPECT_EQ(2, result1);
    EXPECT_EQ(3, result2);
    EXPECT_EQ(4, result3);
}

TEST_P(TestCsvStats, TestGetAvg) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);
    auto pair = stats.GetAvg(2);
    mo::INumericType const& type = static_cast<mo::INumericType const&>(*pair->second);
    std::unique_ptr<std::byte const[]> result(pair->first);
    auto s = type.GetValue<mo::Double>(result.get());
    EXPECT_DOUBLE_EQ(s, 53.1525);
}

TEST_P(TestCsvStats, TestShowSample) {
    CsvStatsParams const& p = GetParam();
    CsvStats stats(p.dataset_, p.sep_, p.has_header_);
    stats.ShowSample(1, 8, 1, 5);
}

INSTANTIATE_TEST_SUITE_P(TypeSystem, TestCsvStats,
                         ::testing::Values(CsvStatsParams("TestCsvStats.csv", ',', false)));

TEST(TestCsvStats, TestGetSTD) {
    CsvStats stats("BernoulliRelation.csv", ',', true);
    auto pair = stats.GetSTD(1);
    mo::INumericType const& type = static_cast<mo::INumericType const&>(*pair->second);
    std::unique_ptr<std::byte const[]> result(pair->first);
    auto s = type.GetValue<mo::Double>(result.get());
    mo::Double expected = 0.547722557505166113456969782801;
    EXPECT_DOUBLE_EQ(s, expected);
}

TEST(TestCsvStats, TestGetSkewness) {
    CsvStats stats("BernoulliRelation.csv", ',', true);
    auto pair = stats.GetSkewness(1);
    mo::INumericType const& type = static_cast<mo::INumericType const&>(*pair->second);
    std::unique_ptr<std::byte const[]> result(pair->first);
    auto s = type.GetValue<mo::Double>(result.get());
    mo::Double expected = 0.0;
    EXPECT_DOUBLE_EQ(s, expected);
}
};  // namespace tests
