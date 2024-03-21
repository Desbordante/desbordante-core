#include <easylogging++.h>
#include <gmock/gmock.h>

#include "algorithms/algo_factory.h"
#include "algorithms/statistics/data_stats.h"
#include "all_csv_configs.h"
#include "config/names.h"

namespace tests {
namespace mo = model;

// to run tests:
// ./Desbordante_test --gtest_filter="*TestDataStats*"

static algos::StdParamsMap GetParamMap(CSVConfig const &csv_config,
                                       bool const is_null_equal_null = true,
                                       ushort thread_num = 1) {
    using namespace config::names;
    return {{kCsvConfig, csv_config}, {kEqualNulls, is_null_equal_null}, {kThreads, thread_num}};
}

static std::unique_ptr<algos::DataStats> MakeStatAlgorithm(CSVConfig const &csv_config,
                                                           bool const is_null_equal_null = true,
                                                           ushort thread_num = 1) {
    return algos::CreateAndLoadAlgorithm<algos::DataStats>(
            GetParamMap(csv_config, is_null_equal_null, thread_num));
}

class TestDataStats : public ::testing::TestCase {};

TEST(TestDataStats, TestNullEmpties) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
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
    EXPECT_FALSE(stats.GetNumberOfZeros(0).HasValue());
    EXPECT_FALSE(stats.GetNumberOfNegatives(0).HasValue());
    EXPECT_FALSE(stats.GetSumOfSquares(0).HasValue());
    EXPECT_FALSE(stats.GetGeometricMean(0).HasValue());
    EXPECT_FALSE(stats.GetMeanAD(0).HasValue());
    EXPECT_FALSE(stats.GetMedian(0).HasValue());
    EXPECT_FALSE(stats.GetMedianAD(0).HasValue());
    EXPECT_FALSE(stats.GetNumberOfNonLetterChars(0).HasValue());
    EXPECT_FALSE(stats.GetNumberOfDigitChars(0).HasValue());
    EXPECT_FALSE(stats.GetNumberOfLowercaseChars(0).HasValue());
    EXPECT_FALSE(stats.GetNumberOfUppercaseChars(0).HasValue());
    EXPECT_FALSE(stats.GetNumberOfChars(0).HasValue());
    EXPECT_FALSE(stats.GetAvgNumberOfChars(0).HasValue());
}

TEST(TestDataStats, TestGetAvgNumberOfChars) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic avg_num_chars_stat = stats.GetAvgNumberOfChars(10);
    double count = mo::Type::GetValue<mo::Double>(avg_num_chars_stat.GetData());
    EXPECT_DOUBLE_EQ(count, 5.875);
}

TEST(TestDataStats, TestGetNumberOfChars) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic num_chars_stat = stats.GetNumberOfChars(10);
    size_t count = mo::Type::GetValue<mo::Int>(num_chars_stat.GetData());
    EXPECT_EQ(count, 47);
}

TEST(TestDataStats, TestGetNumberOfUppercaseChars) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic num_uppercase_chars = stats.GetNumberOfUppercaseChars(10);
    size_t count = mo::Type::GetValue<mo::Int>(num_uppercase_chars.GetData());
    EXPECT_EQ(count, 6);
}

TEST(TestDataStats, TestGetNumberOfLowercaseChars) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic num_lowercase_chars = stats.GetNumberOfLowercaseChars(10);
    size_t count = mo::Type::GetValue<mo::Int>(num_lowercase_chars.GetData());
    EXPECT_EQ(count, 33);
}

TEST(TestDataStats, TestGetNumberOfDigitChars) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic non_letter_chars_stat = stats.GetNumberOfDigitChars(10);
    size_t count = mo::Type::GetValue<mo::Int>(non_letter_chars_stat.GetData());
    EXPECT_EQ(count, 6);
}

TEST(TestDataStats, TestGetNumberOfNonLetterChars) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic non_letter_chars_stat = stats.GetNumberOfNonLetterChars(10);
    size_t count = mo::Type::GetValue<mo::Int>(non_letter_chars_stat.GetData());
    EXPECT_EQ(count, 8);
}

TEST(TestDataStats, TestGetVocab) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic vocab_stat = stats.GetVocab(1);
    std::string str = mo::Type::GetValue<mo::String>(vocab_stat.GetData());
    EXPECT_EQ(str, "abd");
}

TEST(TestDataStats, TestGetNumberOfNulls) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    size_t num_nulls = stats_ptr->GetNumNulls(0);
    EXPECT_EQ(num_nulls, 5);
}

TEST(TestDataStats, TestGetColumnsWithUniqueValues) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    std::vector<size_t> expected_cols = stats_ptr->GetColumnsWithUniqueValues();
    std::vector<size_t> actual_cols = std::vector<size_t>{8, 9, 10};
    EXPECT_EQ(expected_cols, actual_cols);
}

TEST(TestDataStats, TestGetNullColumns) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kSimpleTypes);
    std::vector<size_t> expected_cols = stats_ptr->GetNullColumns();
    std::vector<size_t> actual_cols = std::vector<size_t>{1};
    EXPECT_EQ(expected_cols, actual_cols);
}

TEST(TestDataStats, TestGetColumnsWithNull) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestMetric);
    std::vector<size_t> expected_cols = stats_ptr->GetColumnsWithNull();
    std::vector<size_t> actual_cols = std::vector<size_t>{6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    EXPECT_EQ(expected_cols, actual_cols);
}

TEST(TestDataStats, TestMedianAD) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic median_ad_stat = stats.GetMedianAD(8);
    mo::Double median_ad = mo::Type::GetValue<mo::Double>(median_ad_stat.GetData());
    EXPECT_DOUBLE_EQ(123.0, median_ad);
}

TEST(TestDataStats, TestGetMedian) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    auto test = [&stats = *stats_ptr](size_t index) {
        algos::Statistic median_stat = stats.GetMedian(index);
        mo::Double median = mo::Type::GetValue<mo::Double>(median_stat.GetData());
        return median;
    };
    EXPECT_DOUBLE_EQ(test(8), 35.);
    EXPECT_DOUBLE_EQ(test(9), 25.875);
}

TEST(TestDataStats, TestMeanAD) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic mean_ad_stat = stats.GetMeanAD(7);
    mo::Double mean_ad = mo::Type::GetValue<mo::Double>(mean_ad_stat.GetData());
    EXPECT_DOUBLE_EQ(258.263, mean_ad);
}

TEST(TestDataStats, TestGeometricMean) {
    auto test = [](int index) {
        std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
        algos::DataStats &stats = *stats_ptr;
        algos::Statistic geometric_mean_stat = stats.GetGeometricMean(index);
        mo::Double geometric_mean = mo::Type::GetValue<mo::Double>(geometric_mean_stat.GetData());
        return geometric_mean;
    };
    EXPECT_DOUBLE_EQ(2.4819630489759605, test(3));
    EXPECT_DOUBLE_EQ(33.33024629230983, test(9));
}

TEST(TestDataStats, TestSumOfSquares) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic sum_stat = stats.GetSumOfSquares(7);
    mo::Double sum = mo::Type::GetValue<mo::Double>(sum_stat.GetData());
    EXPECT_DOUBLE_EQ(1096089.607224L, sum);
}

TEST(TestDataStats, TestNumberOfNegatives) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic num_negatives_stat = stats.GetNumberOfNegatives(8);
    mo::Int num_negatives = mo::Type::GetValue<mo::Int>(num_negatives_stat.GetData());
    EXPECT_EQ(3, num_negatives);
}

TEST(TestDataStats, TestGetNumberOfZeros) {
    auto test = [](CSVConfig const &csv_config, size_t index) {
        std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(csv_config);
        algos::DataStats &stats = *stats_ptr;
        algos::Statistic num_zeros_stat = stats.GetNumberOfZeros(index);
        mo::Int num_zeros = mo::Type::GetValue<mo::Int>(num_zeros_stat.GetData());
        return num_zeros;
    };
    EXPECT_EQ(5, test(kBernoulliRelation, 0));  // Int zeros
    EXPECT_EQ(3, test(kTestDataStats, 7));      // Double zeros
}

TEST(TestDataStats, TestMinString) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    auto min_stat = stats.GetMin(1);
    auto min = mo::Type::GetValue<mo::String>(min_stat.GetData());
    EXPECT_EQ("a", min);
}

TEST(TestDataStats, TestMaxString) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    auto max_stat = stats.GetMax(1);
    auto max = mo::Type::GetValue<mo::String>(max_stat.GetData());
    EXPECT_EQ("abd", max);
}

TEST(TestDataStats, TestMinDouble) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    auto min_stat = stats.GetMin(2);
    auto min = mo::Type::GetValue<mo::Double>(min_stat.GetData());
    EXPECT_DOUBLE_EQ(1.07, min);
}

TEST(TestDataStats, TestMaxDouble) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    auto max_stat = stats.GetMax(2);
    auto max = mo::Type::GetValue<mo::Double>(max_stat.GetData());
    EXPECT_DOUBLE_EQ(143.9, max);
}

TEST(TestDataStats, TestSumDouble) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    auto sum_stat = stats.GetSum(2);
    EXPECT_DOUBLE_EQ(212.61, mo::Type::GetValue<mo::Double>(sum_stat.GetData()));
}

TEST(TestDataStats, NumberOfValues) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    EXPECT_EQ(0, stats.NumberOfValues(0));
}

TEST(TestDataStats, TestDistinct) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    auto distinct = stats.Distinct(3);
    EXPECT_EQ(5, distinct);
    EXPECT_EQ(6, stats.Distinct(5));  // mixed column
}

TEST(TestDataStats, TestDistinctStringColumn) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    EXPECT_EQ(7, stats.Distinct(6));
}

TEST(TestDataStats, TestIsCategorial) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    EXPECT_TRUE(stats.IsCategorical(3, 5));
}

TEST(TestDataStats, TestGetQuantiles) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
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
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    auto avg_stat = stats.GetAvg(2);
    auto s = mo::Type::GetValue<mo::Double>(avg_stat.GetData());
    EXPECT_DOUBLE_EQ(s, 53.1525);
}

TEST(TestDataStats, TestShowSample) {
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    std::vector<std::vector<std::string>> sample = stats.ShowSample(1, 8, 1, 5);
    for (auto const &row : sample) {
        std::stringstream result;
        for (auto const &str : row) {
            result << str << " \t";
        }
        LOG(INFO) << result.str();
    }
}

TEST(TestDataStats, TestShowAllStats) {
    // Mixed type statistics will be calculated here.
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    stats.Execute();
    LOG(INFO) << stats.ToString();
}

TEST(TestDataStats, TestGetSTD) {
    auto stats_ptr = MakeStatAlgorithm(kBernoulliRelation);
    algos::DataStats &stats = *stats_ptr;
    auto std_stat = stats.GetCorrectedSTD(1);
    auto s = mo::Type::GetValue<mo::Double>(std_stat.GetData());
    mo::Double expected = 0.547722557505166113456969782801;
    EXPECT_DOUBLE_EQ(s, expected);
}

TEST(TestDataStats, TestGetSkewness) {
    auto stats_ptr = MakeStatAlgorithm(kBernoulliRelation);
    algos::DataStats &stats = *stats_ptr;
    auto skewness_stat = stats.GetSkewness(1);
    auto s = mo::Type::GetValue<mo::Double>(skewness_stat.GetData());
    mo::Double expected = 0.0;
    EXPECT_DOUBLE_EQ(s, expected);
}

TEST(TestDataStats, TestGetKurtosis) {
    auto stats_ptr = MakeStatAlgorithm(kBernoulliRelation);
    algos::DataStats &stats = *stats_ptr;
    auto kurtosis_stat = stats.GetKurtosis(1);
    auto k = mo::Type::GetValue<mo::Double>(kurtosis_stat.GetData());
    mo::Double expected = -2.305;
    EXPECT_NEAR(k, expected, 0.001);
}

TEST(TestDataStats, CorrectExecutionEmpty) {
    auto stats_ptr = MakeStatAlgorithm(kTestEmpty);
    algos::DataStats &stats = *stats_ptr;
    stats.Execute();
    EXPECT_EQ(stats.GetAllStats().size(), 0);
}

// To measure performace of mining statistics in multiple threads.
#if 0
TEST(TestCsvStats, TestDiffThreadNum) {
    for(unsigned thread_num = 1; thread_num < 9; ++thread_num) {
        LOG(INFO) << "thread num = " << thread_num;
        auto start_time = std::chrono::system_clock::now();
        algos::CsvStats stats(MakeConfig(kEpicMeds, true, thread_num));
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

TEST(TestDataStats, MultipleExecutionConsistentResults) {
    auto stats_ptr = MakeStatAlgorithm(kBernoulliRelation);
    stats_ptr->Execute();
    std::string first_res = stats_ptr->ToString();
    for (int i = 0; i < 5; ++i) {
        algos::ConfigureFromMap(*stats_ptr, GetParamMap(kBernoulliRelation));
        stats_ptr->Execute();
        ASSERT_EQ(first_res, stats_ptr->ToString()) << "fail on run " << i;
    }
}

};  // namespace tests
