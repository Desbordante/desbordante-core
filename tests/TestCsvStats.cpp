#include <gmock/gmock.h>

#include "../src/algorithms/statistics/CsvStats.h"

namespace tests {
namespace mo = model;

// to run tests:
//  ./Desbordante_test --gtest_filter="*TestCsvStats*"

FDAlgorithm::Config makeConfig(std::string_view dataset, char const sep = ',',
                               bool const has_header = true, bool const is_null_equal_null = true,
                               unsigned thread_num = 1) {
    FDAlgorithm::Config config;
    config.data = std::filesystem::current_path() / "inputData" / dataset;
    config.has_header = has_header;
    config.separator = sep;
    config.is_null_equal_null = is_null_equal_null;
    config.parallelism = thread_num;
    return config;
}

struct CsvStatsParams {
    FDAlgorithm::Config config;

    CsvStatsParams(std::string_view dataset, char const sep = ',', bool const has_header = true,
                   bool const is_null_equal_null = true) noexcept
        : config(makeConfig(dataset, sep, has_header, is_null_equal_null)) {}
};

class TestCsvStats : public ::testing::TestWithParam<CsvStatsParams> {};

TEST_P(TestCsvStats, TestNullEmpties) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);
    EXPECT_EQ(0, stats.GetMin(0).HasValue());
    EXPECT_EQ(0, stats.GetMax(0).HasValue());
    EXPECT_EQ(0, stats.GetSum(0).HasValue());
    EXPECT_EQ(0, stats.GetAvg(0).HasValue());
    EXPECT_EQ(0, stats.GetSTD(0).HasValue());
    EXPECT_EQ(0, stats.GetSkewness(0).HasValue());
    EXPECT_EQ(0, stats.GetKurtosis(0).HasValue());
    EXPECT_EQ(0, stats.GetQuantile(0.25, 0).HasValue());
    EXPECT_EQ(0, stats.GetQuantile(0.5, 0).HasValue());
    EXPECT_EQ(0, stats.GetQuantile(0.75, 0).HasValue());
}

TEST_P(TestCsvStats, TestMinString) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);
    auto min_stat = stats.GetMin(1);
    auto min = min_stat.GetType()->GetValue<mo::String>(min_stat.GetData());
    EXPECT_EQ("a", min);
}

TEST_P(TestCsvStats, TestMaxString) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);
    auto max_stat = stats.GetMax(1);
    auto max = max_stat.GetType()->GetValue<mo::String>(max_stat.GetData());
    EXPECT_EQ("abd", max);
}

TEST_P(TestCsvStats, TestMinDouble) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);
    auto min_stat = stats.GetMin(2);
    auto min = min_stat.GetType()->GetValue<mo::Double>(min_stat.GetData());
    EXPECT_DOUBLE_EQ(1.07, min);
}

TEST_P(TestCsvStats, TestMaxDouble) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);
    auto max_stat = stats.GetMax(2);
    auto max = max_stat.GetType()->GetValue<mo::Double>(max_stat.GetData());
    EXPECT_DOUBLE_EQ(143.9, max);
}

TEST_P(TestCsvStats, TestSumDouble) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);
    auto sum_stat = stats.GetSum(2);
    EXPECT_DOUBLE_EQ(212.61, sum_stat.GetType()->GetValue<mo::Double>(sum_stat.GetData()));
}

TEST_P(TestCsvStats, TestCount) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);
    int count = stats.Count(0);
    EXPECT_EQ(0, count);
}

TEST_P(TestCsvStats, TestDistinct) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);
    auto distinct = stats.Distinct(3);
    EXPECT_EQ(5, distinct);
}

TEST_P(TestCsvStats, TestIsCategorial) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);
    auto distinct = stats.isCategorical(3, 5);
    EXPECT_EQ(true, distinct);
}

TEST_P(TestCsvStats, TestGetQuantiles) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);

    auto quantile_0_25 = stats.GetQuantile(0.25, 4);
    auto result1 = quantile_0_25.GetType()->GetValue<mo::Int>(quantile_0_25.GetData());
    auto quantile_0_5 = stats.GetQuantile(0.5, 4);
    auto result2 = quantile_0_5.GetType()->GetValue<mo::Int>(quantile_0_5.GetData());
    auto quantile_0_75 = stats.GetQuantile(0.75, 4);
    auto result3 = quantile_0_75.GetType()->GetValue<mo::Int>(quantile_0_75.GetData());

    EXPECT_EQ(2, result1);
    EXPECT_EQ(3, result2);
    EXPECT_EQ(4, result3);
}

TEST_P(TestCsvStats, TestGetAvg) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);
    auto avg_stat = stats.GetAvg(2);
    auto s = avg_stat.GetType()->GetValue<mo::Double>(avg_stat.GetData());
    EXPECT_DOUBLE_EQ(s, 53.1525);
}

TEST_P(TestCsvStats, TestShowSample) {
    CsvStatsParams const& p = GetParam();
    algos::CsvStats stats(p.config);
    stats.ShowSample(1, 8, 1, 5);
}

// TEST_P(TestCsvStats, TestDiffThreadNum) {
//     for(unsigned thread_num = 1; thread_num < 9; ++thread_num) {
//         std::cout << "thread num = " << thread_num << "\n";
//         auto start_time = std::chrono::system_clock::now();
//         algos::CsvStats stats(makeConfig("EpicMeds.csv", '|', true, true, thread_num));
//         auto elapsed_milliseconds =
//             std::chrono::duration_cast<std::chrono::milliseconds>(
//                 std::chrono::system_clock::now() - start_time
//             );
//         std::cout << "Reading time = " << elapsed_milliseconds.count() << "\n";
//         unsigned time = stats.Execute();
//         std::cout << "Executing time = " << time << "\n";
//         //std::cout << stats.toString();
//     }
// }

TEST_P(TestCsvStats, TestShowAllStats) {
    //Mixed type statistics will be calculated here.
    algos::CsvStats stats(GetParam().config);
    stats.Execute();
    std::cout << stats.ToString();
}

INSTANTIATE_TEST_SUITE_P(TypeSystem, TestCsvStats,
                         ::testing::Values(CsvStatsParams("TestCsvStats.csv", ',', false)));

TEST(TestCsvStats, TestGetSTD) {
    algos::CsvStats stats(makeConfig("BernoulliRelation.csv"));
    auto STD_stat = stats.GetSTD(1);
    auto s = STD_stat.GetType()->GetValue<mo::Double>(STD_stat.GetData());
    mo::Double expected = 0.547722557505166113456969782801;
    EXPECT_DOUBLE_EQ(s, expected);
}

TEST(TestCsvStats, TestGetSkewness) {
    algos::CsvStats stats(makeConfig("BernoulliRelation.csv"));
    auto skewness_stat = stats.GetSkewness(1);
    auto s = skewness_stat.GetType()->GetValue<mo::Double>(skewness_stat.GetData());
    mo::Double expected = 0.0;
    EXPECT_DOUBLE_EQ(s, expected);
}
};  // namespace tests
