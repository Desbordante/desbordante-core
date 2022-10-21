#include <gmock/gmock.h>

#include "csv_stats.h"

namespace tests {
namespace mo = model;

// to run tests:
//  ./Desbordante_test --gtest_filter="*TestCsvStats*"

static FDAlgorithm::Config MakeConfig(std::string_view dataset, char const sep = ',',
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

class TestCsvStats : public ::testing::TestCase{};

TEST(TestCsvStats, TestNullEmpties) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    EXPECT_FALSE(stats.GetMin(0).HasValue());
    EXPECT_FALSE(stats.GetMax(0).HasValue());
    EXPECT_FALSE(stats.GetSum(0).HasValue());
    EXPECT_FALSE(stats.GetAvg(0).HasValue());
    EXPECT_FALSE(stats.GetCorrectedSTD(0).HasValue());
    EXPECT_FALSE(stats.GetSkewness(0).HasValue());
    EXPECT_FALSE(stats.GetKurtosis(0).HasValue());
    EXPECT_FALSE(stats.GetQuantile(0.25, 0).HasValue());
    EXPECT_FALSE(stats.GetQuantile(0.5, 0).HasValue());
    EXPECT_FALSE(stats.GetQuantile(0.75, 0).HasValue());
}

TEST(TestCsvStats, TestMinString) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    auto min_stat = stats.GetMin(1);
    auto min = mo::Type::GetValue<mo::String>(min_stat.GetData());
    EXPECT_EQ("a", min);
}

TEST(TestCsvStats, TestMaxString) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    auto max_stat = stats.GetMax(1);
    auto max = mo::Type::GetValue<mo::String>(max_stat.GetData());
    EXPECT_EQ("abd", max);
}

TEST(TestCsvStats, TestMinDouble) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    auto min_stat = stats.GetMin(2);
    auto min = mo::Type::GetValue<mo::Double>(min_stat.GetData());
    EXPECT_DOUBLE_EQ(1.07, min);
}

TEST(TestCsvStats, TestMaxDouble) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    auto max_stat = stats.GetMax(2);
    auto max = mo::Type::GetValue<mo::Double>(max_stat.GetData());
    EXPECT_DOUBLE_EQ(143.9, max);
}

TEST(TestCsvStats, TestSumDouble) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    auto sum_stat = stats.GetSum(2);
    EXPECT_DOUBLE_EQ(212.61, mo::Type::GetValue<mo::Double>(sum_stat.GetData()));
}

TEST(TestCsvStats, NumberOfValues) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    EXPECT_EQ(0, stats.NumberOfValues(0));
}

TEST(TestCsvStats, TestDistinct) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    auto distinct = stats.Distinct(3);
    EXPECT_EQ(5, distinct);
}

TEST(TestCsvStats, TestDistinctStringColumn) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    EXPECT_EQ(7, stats.Distinct(6));
}

TEST(TestCsvStats, TestIsCategorial) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    EXPECT_TRUE(stats.IsCategorical(3, 5));
}

TEST(TestCsvStats, TestGetQuantiles) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));

    auto quantile_0_25 = stats.GetQuantile(0.25, 4);
    auto result1 = mo::Type::GetValue<mo::Int>(quantile_0_25.GetData());
    auto quantile_0_5 = stats.GetQuantile(0.5, 4);
    auto result2 = mo::Type::GetValue<mo::Int>(quantile_0_5.GetData());
    auto quantile_0_75 = stats.GetQuantile(0.75, 4);
    auto result3 = mo::Type::GetValue<mo::Int>(quantile_0_75.GetData());

    EXPECT_EQ(2, result1);
    EXPECT_EQ(3, result2);
    EXPECT_EQ(4, result3);
}

TEST(TestCsvStats, TestGetAvg) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    auto avg_stat = stats.GetAvg(2);
    auto s = mo::Type::GetValue<mo::Double>(avg_stat.GetData());
    EXPECT_DOUBLE_EQ(s, 53.1525);
}

TEST(TestCsvStats, TestShowSample) {
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    std::vector<std::vector<std::string>> sample = stats.ShowSample(1, 8, 1, 5);
    for(const auto& row : sample) {
        for(const auto& str : row) {
            std::cout << str << " \t";
        }
        std::cout << '\n';
    }
}

TEST(TestCsvStats, TestShowAllStats) {
    // Mixed type statistics will be calculated here.
    algos::CsvStats stats(MakeConfig("TestCsvStats.csv",  ',', false));
    stats.Execute();
    std::cout << stats.ToString();
}

TEST(TestCsvStats, TestGetSTD) {
    algos::CsvStats stats(MakeConfig("BernoulliRelation.csv"));
    auto STD_stat = stats.GetCorrectedSTD(1);
    auto s = mo::Type::GetValue<mo::Double>(STD_stat.GetData());
    mo::Double expected = 0.547722557505166113456969782801;
    EXPECT_DOUBLE_EQ(s, expected);
}

TEST(TestCsvStats, TestGetSkewness) {
    algos::CsvStats stats(MakeConfig("BernoulliRelation.csv"));
    auto skewness_stat = stats.GetSkewness(1);
    auto s = mo::Type::GetValue<mo::Double>(skewness_stat.GetData());
    mo::Double expected = 0.0;
    EXPECT_DOUBLE_EQ(s, expected);
}

//To measure performace of mining statistics in multiple threads.
#if 0
TEST(TestCsvStats, TestDiffThreadNum) {
    for(unsigned thread_num = 1; thread_num < 9; ++thread_num) {
        std::cout << "thread num = " << thread_num << "\n";
        auto start_time = std::chrono::system_clock::now();
        algos::CsvStats stats(MakeConfig("EpicMeds.csv", '|', true, true, thread_num));
        auto elapsed_milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - start_time
            );
        std::cout << "Reading time = " << elapsed_milliseconds.count() << "\n";
        unsigned time = stats.Execute();
        std::cout << "Executing time = " << time << "\n";
        //std::cout << stats.toString();
    }
}
#endif

};  // namespace tests
