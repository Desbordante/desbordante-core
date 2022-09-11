#include <gmock/gmock.h>

#include "CsvStats.h"

namespace tests {
namespace mo = model;

//to run tests:
// ./Desbordante_test --gtest_filter="*TestCsvStats*"

FDAlgorithm::Config makeConfig(std::string_view dataset, char const sep = ',',
        bool const has_header = true, bool const is_null_equal_null = true) {
    FDAlgorithm::Config config;
    config.data = std::filesystem::current_path() / "inputData" / dataset;
    config.has_header = has_header;
    config.separator = sep;
    config.is_null_equal_null = is_null_equal_null;
    return config;
}

struct CsvStatsParams {
    FDAlgorithm::Config config;

    CsvStatsParams(std::string_view dataset, char const sep = ',',
        bool const has_header = true, bool const is_null_equal_null = true) 
        noexcept: config(makeConfig(dataset, sep, has_header, is_null_equal_null)) {}
};

class TestCsvStats : public ::testing::TestWithParam<CsvStatsParams> {};

TEST_P(TestCsvStats, TestNullEmpties) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    EXPECT_EQ(0, stats.GetMin(0).hasValue());
    EXPECT_EQ(0, stats.GetMax(0).hasValue());
    EXPECT_EQ(0, stats.GetSum(0).hasValue());
    EXPECT_EQ(0, stats.GetAvg(0).hasValue());
    EXPECT_EQ(0, stats.GetSTD(0).hasValue());
    EXPECT_EQ(0, stats.GetSkewness(0).hasValue());
    EXPECT_EQ(0, stats.GetKurtosis(0).hasValue());
    EXPECT_EQ(0, stats.GetQuantile(0.25, 0).hasValue());
    EXPECT_EQ(0, stats.GetQuantile(0.5, 0).hasValue());
    EXPECT_EQ(0, stats.GetQuantile(0.75, 0).hasValue());
}

TEST_P(TestCsvStats, TestMinString) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    auto pair = stats.GetMin(1);
    mo::Type const& type = static_cast<mo::Type const&>(*pair.getType());
    auto min = type.GetValue<mo::String>(pair.getData());
    EXPECT_EQ("a", min);
    pair.Free();
}

TEST_P(TestCsvStats, TestMaxString) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    auto pair = stats.GetMax(1);
    mo::Type const& type = static_cast<mo::Type const&>(*pair.getType());
    auto max = type.GetValue<mo::String>(pair.getData());
    EXPECT_EQ("abd", max);
    pair.Free();
}

TEST_P(TestCsvStats, TestMinDouble) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    auto pair = stats.GetMin(2);
    mo::Type const& type = static_cast<mo::Type const&>(*pair.getType());
    auto min = type.GetValue<mo::Double>(pair.getData());
    EXPECT_DOUBLE_EQ(1.07, min);
    pair.Free();
}

TEST_P(TestCsvStats, TestMaxDouble) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    auto pair = stats.GetMax(2);
    mo::Type const& type = static_cast<mo::Type const&>(*pair.getType());
    auto max = type.GetValue<mo::Double>(pair.getData());
    EXPECT_DOUBLE_EQ(143.9, max);
    pair.Free();
}

TEST_P(TestCsvStats, TestSumDouble) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    auto pair = stats.GetSum(2);
    mo::INumericType const& type = static_cast<mo::INumericType const&>(*pair.getType());
    std::unique_ptr<std::byte const[]> sum(pair.getDataAndFree());
    EXPECT_DOUBLE_EQ(212.61, type.GetValue<mo::Double>(sum.get()));
}
TEST_P(TestCsvStats, TestCount) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    int count = stats.Count(0);
    EXPECT_EQ(0, count);
}
TEST_P(TestCsvStats, TestDistinct) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    auto distinct = stats.Distinct(3);
    EXPECT_EQ(5, distinct);
}

TEST_P(TestCsvStats, TestIsCategorial) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    auto distinct = stats.isCategorial(3, 5);
    EXPECT_EQ(true, distinct);
}

TEST_P(TestCsvStats, TestGetQuantiles) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);

    auto quantile_0_25 = stats.GetQuantile(0.25, 4);
    auto result1 = quantile_0_25.getType()->GetValue<mo::Int>(quantile_0_25.getData());
    auto quantile_0_5 = stats.GetQuantile(0.5, 4);
    auto result2 = quantile_0_5.getType()->GetValue<mo::Int>(quantile_0_5.getData());
    auto quantile_0_75 = stats.GetQuantile(0.75, 4);
    auto result3 = quantile_0_75.getType()->GetValue<mo::Int>(quantile_0_75.getData());

    EXPECT_EQ(2, result1);
    EXPECT_EQ(3, result2);
    EXPECT_EQ(4, result3);
    quantile_0_25.Free();
    quantile_0_5.Free();
    quantile_0_75.Free();
}

TEST_P(TestCsvStats, TestGetAvg) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    auto pair = stats.GetAvg(2);
    mo::INumericType const& type = static_cast<mo::INumericType const&>(*pair.getType());
    std::unique_ptr<std::byte const[]> result(pair.getDataAndFree());
    auto s = type.GetValue<mo::Double>(result.get());
    EXPECT_DOUBLE_EQ(s, 53.1525);
}

TEST_P(TestCsvStats, TestShowSample) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    stats.ShowSample(1, 8, 1, 5);
}

TEST_P(TestCsvStats, TestShowAllStats) {
    CsvStatsParams const& p = GetParam();
    statistics::CsvStats stats(p.config);
    std::cout << stats.toString();
}

INSTANTIATE_TEST_SUITE_P(TypeSystem, TestCsvStats, ::testing::Values(
    CsvStatsParams("TestCsvStats.csv", ',', false)));

TEST(TestCsvStats, TestGetSTD) {
    statistics::CsvStats stats(makeConfig("BernoulliRelation.csv"));
    auto pair = stats.GetSTD(1);
    mo::INumericType const& type = static_cast<mo::INumericType const&>(*pair.getType());
    std::unique_ptr<std::byte const[]> result(pair.getDataAndFree());
    auto s = type.GetValue<mo::Double>(result.get());
    mo::Double expected = 0.547722557505166113456969782801;
    EXPECT_DOUBLE_EQ(s, expected);
}

TEST(TestCsvStats, TestGetSkewness) {
    statistics::CsvStats stats(makeConfig("BernoulliRelation.csv"));
    auto pair = stats.GetSkewness(1);
    mo::INumericType const& type = static_cast<mo::INumericType const&>(*pair.getType());
    std::unique_ptr<std::byte const[]> result(pair.getDataAndFree());
    auto s = type.GetValue<mo::Double>(result.get());
    mo::Double expected = 0.0;
    EXPECT_DOUBLE_EQ(s, expected);
}
};  // namespace tests
