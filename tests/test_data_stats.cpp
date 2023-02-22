#include <easylogging++.h>
#include <gmock/gmock.h>

#include "algorithms/algo_factory.h"
#include "algorithms/statistics/data_stats.h"

namespace tests {
namespace mo = model;

// to run tests:
// ./Desbordante_test --gtest_filter="*TestDataStats*"

static std::unique_ptr<algos::DataStats> MakeStatPrimitive(std::string_view dataset,
                                                           char const separator = ',',
                                                           bool const has_header = true,
                                                           bool const is_null_equal_null = true,
                                                           ushort thread_num = 1) {
    algos::StdParamsMap params{
            {algos::config::names::kData,
             std::string{std::filesystem::current_path() / "input_data" / dataset}},
            {algos::config::names::kHasHeader, has_header},
            {algos::config::names::kSeparator, separator},
            {algos::config::names::kEqualNulls, is_null_equal_null},
            {algos::config::names::kThreads, thread_num}};
    return algos::CreateAndLoadPrimitive<algos::DataStats>(params);
}

class TestDataStats : public ::testing::TestCase{};

TEST(TestDataStats, TestNullEmpties) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
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

TEST(TestDataStats, TestMinString) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    auto min_stat = stats.GetMin(1);
    auto min = mo::Type::GetValue<mo::String>(min_stat.GetData());
    EXPECT_EQ("a", min);
}

TEST(TestDataStats, TestMaxString) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    auto max_stat = stats.GetMax(1);
    auto max = mo::Type::GetValue<mo::String>(max_stat.GetData());
    EXPECT_EQ("abd", max);
}

TEST(TestDataStats, TestMinDouble) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    auto min_stat = stats.GetMin(2);
    auto min = mo::Type::GetValue<mo::Double>(min_stat.GetData());
    EXPECT_DOUBLE_EQ(1.07, min);
}

TEST(TestDataStats, TestMaxDouble) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    auto max_stat = stats.GetMax(2);
    auto max = mo::Type::GetValue<mo::Double>(max_stat.GetData());
    EXPECT_DOUBLE_EQ(143.9, max);
}

TEST(TestDataStats, TestSumDouble) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    auto sum_stat = stats.GetSum(2);
    EXPECT_DOUBLE_EQ(212.61, mo::Type::GetValue<mo::Double>(sum_stat.GetData()));
}

TEST(TestDataStats, NumberOfValues) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    EXPECT_EQ(0, stats.NumberOfValues(0));
}

TEST(TestDataStats, TestDistinct) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    auto distinct = stats.Distinct(3);
    EXPECT_EQ(5, distinct);
    EXPECT_EQ(6, stats.Distinct(5)); // mixed column
}

TEST(TestDataStats, TestDistinctStringColumn) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    EXPECT_EQ(7, stats.Distinct(6));
}

TEST(TestDataStats, TestIsCategorial) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    EXPECT_TRUE(stats.IsCategorical(3, 5));
}

TEST(TestDataStats, TestGetQuantiles) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;

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

TEST(TestDataStats, TestGetAvg) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    auto avg_stat = stats.GetAvg(2);
    auto s = mo::Type::GetValue<mo::Double>(avg_stat.GetData());
    EXPECT_DOUBLE_EQ(s, 53.1525);
}

TEST(TestDataStats, TestShowSample) {
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    std::vector<std::vector<std::string>> sample = stats.ShowSample(1, 8, 1, 5);
    for(const auto& row : sample) {
        std::stringstream result;
        for(const auto& str : row) {
            result << str << " \t";
        }
        LOG(INFO) << result.str();
    }
}

TEST(TestDataStats, TestShowAllStats) {
    // Mixed type statistics will be calculated here.
    auto stats_ptr = MakeStatPrimitive("TestCsvStats.csv", ',', false);
    algos::DataStats &stats = *stats_ptr;
    stats.Execute();
    LOG(INFO) << stats.ToString();
}

TEST(TestDataStats, TestGetSTD) {
    auto stats_ptr = MakeStatPrimitive("BernoulliRelation.csv");
    algos::DataStats &stats = *stats_ptr;
    auto STD_stat = stats.GetCorrectedSTD(1);
    auto s = mo::Type::GetValue<mo::Double>(STD_stat.GetData());
    mo::Double expected = 0.547722557505166113456969782801;
    EXPECT_DOUBLE_EQ(s, expected);
}

TEST(TestDataStats, TestGetSkewness) {
    auto stats_ptr = MakeStatPrimitive("BernoulliRelation.csv");
    algos::DataStats &stats = *stats_ptr;
    auto skewness_stat = stats.GetSkewness(1);
    auto s = mo::Type::GetValue<mo::Double>(skewness_stat.GetData());
    mo::Double expected = 0.0;
    EXPECT_DOUBLE_EQ(s, expected);
}

TEST(TestDataStats, CorrectExecutionEmpty) {
    auto stats_ptr = MakeStatPrimitive("TestEmpty.csv");
    algos::DataStats &stats = *stats_ptr;
    stats.Execute();
    EXPECT_EQ(stats.GetAllStats().size(), 0);
}

//To measure performace of mining statistics in multiple threads.
#if 0
TEST(TestCsvStats, TestDiffThreadNum) {
    for(unsigned thread_num = 1; thread_num < 9; ++thread_num) {
        LOG(INFO) << "thread num = " << thread_num;
        auto start_time = std::chrono::system_clock::now();
        algos::CsvStats stats(MakeConfig("EpicMeds.csv", '|', true, true, thread_num));
        auto elapsed_milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - start_time
            );
        LOG(INFO) << "Reading time = " << elapsed_milliseconds.count();
        unsigned time = stats.Execute();
        LOG(INFO) << "Executing time = " << time;
        //LOG(INFO) << stats.toString();
    }
}
#endif

};  // namespace tests
