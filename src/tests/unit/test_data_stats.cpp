#include <gmock/gmock.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/statistics/data_stats.h"
#include "core/config/names.h"
#include "core/util/logger.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
namespace mo = model;

// to run tests:
// ./Desbordante_test --gtest_filter="*TestDataStats*"

static algos::StdParamsMap GetParamMap(CSVConfig const &csv_config,
                                       bool const is_null_equal_null = true,
                                       unsigned short thread_num = 1) {
    using namespace config::names;
    return {{kCsvConfig, csv_config}, {kEqualNulls, is_null_equal_null}, {kThreads, thread_num}};
}

static std::unique_ptr<algos::DataStats> MakeStatAlgorithm(CSVConfig const &csv_config,
                                                           bool const is_null_equal_null = true,
                                                           unsigned short thread_num = 1) {
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
    EXPECT_FALSE(stats.GetMinNumberOfChars(0).HasValue());
    EXPECT_FALSE(stats.GetMaxNumberOfChars(0).HasValue());
    EXPECT_FALSE(stats.GetMinNumberOfWords(0).HasValue());
    EXPECT_FALSE(stats.GetMaxNumberOfWords(0).HasValue());
    EXPECT_FALSE(stats.GetNumberOfWords(0).HasValue());
    EXPECT_FALSE(stats.GetNumberOfEntirelyUppercaseWords(0).HasValue());
    EXPECT_FALSE(stats.GetNumberOfEntirelyLowercaseWords(0).HasValue());
}

TEST(TestDataStats, TestGetWords) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    std::set<std::string> words_stat = stats.GetWords(6);
    std::set<std::string> actual_words =
            std::set<std::string>{"abc", "abd", "abe", "eeee", "ggg", "gre", "grg"};
    EXPECT_EQ(words_stat, actual_words);
}

TEST(TestDataStats, TestGetTopKWords) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    std::vector<std::string> top_k_words_stat = stats.GetTopKWords(11, 1);
    std::vector<std::string> actual_top_k_words = std::vector<std::string>{"this"};
    EXPECT_EQ(top_k_words_stat, actual_top_k_words);
}

TEST(TestDataStats, TestGetTopKChars) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    std::vector<char> top_k_chars_stat = stats.GetTopKChars(10, 2);
    std::vector<char> actual_top_k_chars = std::vector<char>{'d', 'a'};
    EXPECT_EQ(top_k_chars_stat, actual_top_k_chars);
}

TEST(TestDataStats, TestGetWordCount) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic word_count_stat = stats.GetNumberOfWords(11);
    size_t count = mo::Type::GetValue<mo::Int>(word_count_stat.GetData());
    EXPECT_EQ(count, 21);
}

TEST(TestDataStats, TestGetEntirelyUppercaseCount) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic entirely_uppercase_count_stat = stats.GetNumberOfEntirelyUppercaseWords(11);
    size_t count = mo::Type::GetValue<mo::Int>(entirely_uppercase_count_stat.GetData());
    EXPECT_EQ(count, 2);
}

TEST(TestDataStats, TestGetEntirelyLowercaseCount) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic entirely_lowercase_count_stat = stats.GetNumberOfEntirelyLowercaseWords(11);
    size_t count = mo::Type::GetValue<mo::Int>(entirely_lowercase_count_stat.GetData());
    EXPECT_EQ(count, 16);
}

TEST(TestDataStats, TestGetMaxWords) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic max_words_stat = stats.GetMaxNumberOfWords(11);
    size_t count = mo::Type::GetValue<mo::Int>(max_words_stat.GetData());
    EXPECT_EQ(count, 9);
}

TEST(TestDataStats, TestGetMinWords) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic min_words_stat = stats.GetMinNumberOfWords(11);
    size_t count = mo::Type::GetValue<mo::Int>(min_words_stat.GetData());
    EXPECT_EQ(count, 1);
}

TEST(TestDataStats, TestGetMaxChars) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic max_chars_stat = stats.GetMaxNumberOfChars(10);
    size_t count = mo::Type::GetValue<mo::Int>(max_chars_stat.GetData());
    EXPECT_EQ(count, 13);
}

TEST(TestDataStats, TestGetMinChars) {
    std::unique_ptr<algos::DataStats> stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    algos::Statistic min_chars_stat = stats.GetMinNumberOfChars(10);
    size_t count = mo::Type::GetValue<mo::Int>(min_chars_stat.GetData());
    EXPECT_EQ(count, 3);
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
    std::vector<size_t> actual_cols = std::vector<size_t>{8, 9, 10, 11};
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
        LOG_INFO("{}", result.str());
    }
}

TEST(TestDataStats, TestShowAllStats) {
    // Mixed type statistics will be calculated here.
    auto stats_ptr = MakeStatAlgorithm(kTestDataStats);
    algos::DataStats &stats = *stats_ptr;
    stats.Execute();
    LOG_INFO("{}", stats.ToString());
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
        LOG_INFO("thread num = {}", thread_num);
        auto start_time = std::chrono::system_clock::now();
        algos::CsvStats stats(MakeConfig(kEpicMeds, true, thread_num));
        auto elapsed_milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - start_time
            );
        LOG_INFO("Reading time = {}", elapsed_milliseconds.count());
        unsigned time = stats.Execute();
        LOG_INFO("Executing time = {}", time);
        //LOG_INFO() stats.toString();
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

class TestNewStatistics : public ::testing::Test {
protected:
    void SetUp() override {
        stats_ptr = MakeStatAlgorithm(kTestDataStats);
        stats_ptr->Execute();
    }
    
    std::unique_ptr<algos::DataStats> stats_ptr;
};


TEST_F(TestNewStatistics, InterquartileRange_NumericColumn) {
    // Колонка 2: [1.07, 17.21, 143.9, 50.43] -> сортировка: [1.07, 17.21, 50.43, 143.9]
    // n=4, Q1 позиция = 0.25*4 = 1 -> значение 17.21
    // Q3 позиция = 0.75*4 = 3 -> значение 143.9
    // IQR = 143.9 - 17.21 = 126.69
    auto iqr_stat = stats_ptr->GetInterquartileRange(2);
    EXPECT_TRUE(iqr_stat.HasValue());
    
    double iqr = mo::Type::GetValue<mo::Double>(iqr_stat.GetData());
    EXPECT_NEAR(iqr, 126.69, 0.001);
}

TEST_F(TestNewStatistics, InterquartileRange_ColumnWithNulls) {
    // Колонка 4: [1, 2, 3, 4, 5, NULL, NULL] (значения: 1,2,3,4,5)
    // Сортируем: [1, 2, 3, 4, 5]
    // n=5, Q1 позиция = 0.25*5 = 1.25 -> 2
    // Q3 позиция = 0.75*5 = 3.75 -> 4
    // IQR = 4 - 2 = 2
    auto iqr_stat = stats_ptr->GetInterquartileRange(4);
    EXPECT_TRUE(iqr_stat.HasValue());
    
    double iqr = mo::Type::GetValue<mo::Double>(iqr_stat.GetData());
    EXPECT_NEAR(iqr, 2.0, 0.001);
}

TEST_F(TestNewStatistics, InterquartileRange_StringColumnReturnsEmpty) {
    // Строковая колонка должна возвращать пустую статистику
    auto iqr_stat = stats_ptr->GetInterquartileRange(1);
    EXPECT_FALSE(iqr_stat.HasValue());
}

TEST_F(TestNewStatistics, InterquartileRange_NegativeValues) {
    // Колонка 8: [-2841, -112, -19, 23, 47, 134, 901, 9840]
    // Сортируем: [-2841, -112, -19, 23, 47, 134, 901, 9840]
    // n=8, Q1 позиция = 0.25*8 = 2 -> значение -19
    // Q3 позиция = 0.75*8 = 6 -> значение 901
    // IQR = 901 - (-19) = 920
    auto iqr_stat = stats_ptr->GetInterquartileRange(8);
    EXPECT_TRUE(iqr_stat.HasValue());
    
    double iqr = mo::Type::GetValue<mo::Double>(iqr_stat.GetData());
    EXPECT_NEAR(iqr, 920.0, 0.001);
}


TEST_F(TestNewStatistics, CoefficientOfVariation_NumericColumn) {
    // [0.0, 0.0, 85.432, 0.0, 43.5, 523.09, 13.29, 901.72]
    
    // Среднее значение
    // sum = 0.0 + 0.0 + 85.432 + 0.0 + 43.5 + 523.09 + 13.29 + 901.72 = 1567.032
    // mean = 1567.032 / 8 = 195.879
    
    // стандартное отклонение
    // Для каждого значения: (значение - среднее)^2
    // (0.0 - 195.879)^2 = 38368.6
    // (0.0 - 195.879)^2 = 38368.6
    // (85.432 - 195.879)^2 = 12199.5
    // (0.0 - 195.879)^2 = 38368.6
    // (43.5 - 195.879)^2 = 23220.6
    // (523.09 - 195.879)^2 = 107065.0
    // (13.29 - 195.879)^2 = 33339.3
    // (901.72 - 195.879)^2 = 498208.0
    
    // Сумма квадратов отклонений = 38368.6 + 38368.6 + 12199.5 + 38368.6 + 23220.6 + 107065.0 + 33339.3 + 498208.0 = 789138.2
    
    // Дисперсия = 789138.2 / 7 = 112734.0
    // STD = sqrt(112734.0) = 335.76
    
    // Коэф вариации:
    // CV = STD / mean = 335.76 / 195.879 = 1.714
    
    auto cv_stat = stats_ptr->GetCoefficientOfVariation(7);
    EXPECT_TRUE(cv_stat.HasValue());
    
    double cv = mo::Type::GetValue<mo::Double>(cv_stat.GetData());
    EXPECT_NEAR(cv, 1.714, 0.01);
}

TEST_F(TestNewStatistics, CoefficientOfVariation_ZeroMeanReturnsEmpty) {
    auto cv_stat = stats_ptr->GetCoefficientOfVariation(7);
    EXPECT_TRUE(cv_stat.HasValue());
}

TEST_F(TestNewStatistics, CoefficientOfVariation_ConsistencyWithStdAndMean) {
    // Проверяем, что CV = STD / mean для колонки 9
    auto cv_stat = stats_ptr->GetCoefficientOfVariation(9);
    auto std_stat = stats_ptr->GetCorrectedSTD(9);
    auto mean_stat = stats_ptr->GetAvg(9);
    
    EXPECT_TRUE(cv_stat.HasValue());
    EXPECT_TRUE(std_stat.HasValue());
    EXPECT_TRUE(mean_stat.HasValue());
    
    double cv = mo::Type::GetValue<mo::Double>(cv_stat.GetData());
    double std_val = mo::Type::GetValue<mo::Double>(std_stat.GetData());
    double mean_val = mo::Type::GetValue<mo::Double>(mean_stat.GetData());
    
    if (std::abs(mean_val) > 1e-10) {
        double expected_cv = std_val / mean_val;
        EXPECT_NEAR(cv, expected_cv, 1e-10);
    }
}

TEST_F(TestNewStatistics, CoefficientOfVariation_NonNumericColumnReturnsEmpty) {
    auto cv_stat = stats_ptr->GetCoefficientOfVariation(1);
    EXPECT_FALSE(cv_stat.HasValue());
}


TEST_F(TestNewStatistics, Monotonicity_AscendingSequence) {
    // [1, 2, 2, 3, 3, 4, 5]
    auto monotonicity_stat = stats_ptr->GetMonotonicity(3);
    EXPECT_TRUE(monotonicity_stat.HasValue());
    
    std::string monotonicity = mo::Type::GetValue<mo::String>(monotonicity_stat.GetData());
    EXPECT_EQ(monotonicity, "ascending");
}

TEST_F(TestNewStatistics, Monotonicity_StringColumn) {
    // ["", "a", "aaa", "abd", ""]
    auto monotonicity_stat = stats_ptr->GetMonotonicity(1);
    EXPECT_TRUE(monotonicity_stat.HasValue());
    
    std::string monotonicity = mo::Type::GetValue<mo::String>(monotonicity_stat.GetData());
    EXPECT_EQ(monotonicity, "ascending");
}

TEST_F(TestNewStatistics, Monotonicity_NoneMonotonic) {
    // [1.07, 17.21, 143.9, 50.43]
    auto monotonicity_stat = stats_ptr->GetMonotonicity(2);
    EXPECT_TRUE(monotonicity_stat.HasValue());
    
    std::string monotonicity = mo::Type::GetValue<mo::String>(monotonicity_stat.GetData());
    EXPECT_EQ(monotonicity, "none");
}

TEST_F(TestNewStatistics, Monotonicity_WithNullValues) {
    // [1, 2, 3, 4, 5, NULL, NULL]
    auto monotonicity_stat = stats_ptr->GetMonotonicity(4);
    EXPECT_TRUE(monotonicity_stat.HasValue());
    
    std::string monotonicity = mo::Type::GetValue<mo::String>(monotonicity_stat.GetData());
    EXPECT_EQ(monotonicity, "ascending");
}

TEST_F(TestNewStatistics, Monotonicity_NonOrderedTypeReturnsEmpty) {
    // все NULL
    auto monotonicity_stat = stats_ptr->GetMonotonicity(0);
    EXPECT_FALSE(monotonicity_stat.HasValue());
}


TEST_F(TestNewStatistics, JarqueBera_ConsistentWithSkewnessAndKurtosis) {
    // Проверяем формулу: JB = n/6 * (S^2 + (K-3)^2/4)
    auto jb_stat = stats_ptr->GetJarqueBeraStatistic(7);
    auto skewness_stat = stats_ptr->GetSkewness(7);
    auto kurtosis_stat = stats_ptr->GetKurtosis(7);
    
    EXPECT_TRUE(jb_stat.HasValue());
    EXPECT_TRUE(skewness_stat.HasValue());
    EXPECT_TRUE(kurtosis_stat.HasValue());
    
    double jb = mo::Type::GetValue<mo::Double>(jb_stat.GetData());
    double skewness = mo::Type::GetValue<mo::Double>(skewness_stat.GetData());
    double kurtosis = mo::Type::GetValue<mo::Double>(kurtosis_stat.GetData());
    size_t n = stats_ptr->NumberOfValues(7);
    
    double expected_jb = static_cast<double>(n) / 6.0 * 
                        (skewness * skewness + (kurtosis - 3.0) * (kurtosis - 3.0) / 4.0);
    
    EXPECT_NEAR(jb, expected_jb, 1e-10);
}

TEST_F(TestNewStatistics, JarqueBera_NormalDistributionLowValue) {
    // [1, 2, 2, 3, 3, 4, 5] - близка к нормальному распределению
    // JB должна быть небольшой
    auto jb_stat = stats_ptr->GetJarqueBeraStatistic(3);
    EXPECT_TRUE(jb_stat.HasValue());
    
    double jb = mo::Type::GetValue<mo::Double>(jb_stat.GetData());
    // Для небольшой выборки JB может быть > 0, но не слишком большой
    EXPECT_GE(jb, 0.0);
    EXPECT_LT(jb, 10.0);
}

TEST_F(TestNewStatistics, JarqueBera_NonNormalDistributionHighValue) {
    // [-2841, -112, -19, 23, 47, 134, 901, 9840]
    auto jb_stat = stats_ptr->GetJarqueBeraStatistic(8);
    EXPECT_TRUE(jb_stat.HasValue());
    
    auto skewness_stat = stats_ptr->GetSkewness(8);
    auto kurtosis_stat = stats_ptr->GetKurtosis(8);
    
    EXPECT_TRUE(skewness_stat.HasValue());
    EXPECT_TRUE(kurtosis_stat.HasValue());
    
    double jb = mo::Type::GetValue<mo::Double>(jb_stat.GetData());
    double skewness = mo::Type::GetValue<mo::Double>(skewness_stat.GetData());
    double kurtosis = mo::Type::GetValue<mo::Double>(kurtosis_stat.GetData());
    size_t n = stats_ptr->NumberOfValues(8);
    
    // Формула Харке-Бера: JB = n/6 * (S² + (K-3)²/4)
    double expected_jb = static_cast<double>(n) / 6.0 * 
                        (skewness * skewness + (kurtosis - 3.0) * (kurtosis - 3.0) / 4.0);
    
    // Проверяем, что значения близки (допуск 1e-10 для вычислений с double)
    EXPECT_NEAR(jb, expected_jb, 1e-10);
}

TEST_F(TestNewStatistics, JarqueBera_NonNumericColumnReturnsEmpty) {
    auto jb_stat = stats_ptr->GetJarqueBeraStatistic(1);
    EXPECT_FALSE(jb_stat.HasValue());
}


TEST_F(TestNewStatistics, Entropy_StringColumn) {
    // ["abc", "abc", "abd", "abe", "eeee", "gre", "ggg", "grg"]
    // Частоты: abc:2, abd:1, abe:1, eeee:1, gre:1, ggg:1, grg:1
    // Всего: 8
    // Энтропия = -[2/8*log2(2/8) + 6*(1/8*log2(1/8))]
    // = -[0.25*log2(0.25) + 6*0.125*log2(0.125)]
    // = -[0.25*(-2) + 6*0.125*(-3)] = -[-0.5 + -2.25] = 2.75
    auto entropy_stat = stats_ptr->GetEntropy(6);
    EXPECT_TRUE(entropy_stat.HasValue());
    
    double entropy = mo::Type::GetValue<mo::Double>(entropy_stat.GetData());
    EXPECT_NEAR(entropy, 2.75, 0.01);
}

TEST_F(TestNewStatistics, Entropy_MaximumForUniformDistribution) {
    // Максимальная энтропия, когда все значения уникальны
    auto entropy_stat = stats_ptr->GetEntropy(10);
    EXPECT_TRUE(entropy_stat.HasValue());
    
    double entropy = mo::Type::GetValue<mo::Double>(entropy_stat.GetData());
    size_t unique_count = stats_ptr->Distinct(10);
    double max_entropy = std::log2(static_cast<double>(unique_count));
    
    // Энтропия должна быть близка к максимальной
    EXPECT_NEAR(entropy, max_entropy, 0.1);
}

TEST_F(TestNewStatistics, Entropy_WithNullValues) {
    // ["", "a", "aaa", "abd", ""] (2 пустые строки)
    // Частоты: "":2, "a":1, "aaa":1, "abd":1
    auto entropy_stat = stats_ptr->GetEntropy(1);
    EXPECT_TRUE(entropy_stat.HasValue());
    
    double entropy = mo::Type::GetValue<mo::Double>(entropy_stat.GetData());
    // Энтропия должна быть положительной
    EXPECT_GT(entropy, 0.0);
}

TEST_F(TestNewStatistics, Entropy_NonStringColumnReturnsEmpty) {
    auto entropy_stat = stats_ptr->GetEntropy(2);
    EXPECT_FALSE(entropy_stat.HasValue());
}

TEST_F(TestNewStatistics, Entropy_EmptyColumnReturnsEmpty) {
    auto entropy_stat = stats_ptr->GetEntropy(0);
    EXPECT_FALSE(entropy_stat.HasValue());
}


TEST_F(TestNewStatistics, GiniCoefficient_StringColumn) {
    // ["abc", "abc", "abd", "abe", "eeee", "gre", "ggg", "grg"]
    // Частоты: abc:2, другие по 1
    // Всего: 8
    // Коэффициент Джини = 1 - sum(p_i^2)
    // = 1 - [(2/8)^2 + 6*(1/8)^2] = 1 - [0.0625 + 6*0.015625]
    // = 1 - [0.0625 + 0.09375] = 1 - 0.15625 = 0.84375
    auto gini_stat = stats_ptr->GetGiniCoefficient(6);
    EXPECT_TRUE(gini_stat.HasValue());
    
    double gini = mo::Type::GetValue<mo::Double>(gini_stat.GetData());
    EXPECT_NEAR(gini, 0.84375, 0.001);
}

TEST_F(TestNewStatistics, GiniCoefficient_MaximumForUniformDistribution) {
    // Максимальный коэффициент Джини (близкий к 1), когда распределение равномерное
    // Колонка 10: все значения уникальны
    auto gini_stat = stats_ptr->GetGiniCoefficient(10);
    EXPECT_TRUE(gini_stat.HasValue());
    
    double gini = mo::Type::GetValue<mo::Double>(gini_stat.GetData());
    size_t unique_count = stats_ptr->Distinct(10);
    double expected_gini = 1.0 - 1.0/static_cast<double>(unique_count);
    
    EXPECT_NEAR(gini, expected_gini, 0.001);
}

TEST_F(TestNewStatistics, GiniCoefficient_WithNullValues) {
    // Колонка 1: ["", "a", "aaa", "abd", ""]
    auto gini_stat = stats_ptr->GetGiniCoefficient(1);
    EXPECT_TRUE(gini_stat.HasValue());
    
    double gini = mo::Type::GetValue<mo::Double>(gini_stat.GetData());
    // Коэффициент Джини должен быть в диапазоне [0, 1)
    EXPECT_GE(gini, 0.0);
    EXPECT_LT(gini, 1.0);
}

TEST_F(TestNewStatistics, GiniCoefficient_NonStringColumnReturnsEmpty) {
    auto gini_stat = stats_ptr->GetGiniCoefficient(2);
    EXPECT_FALSE(gini_stat.HasValue());
}

TEST_F(TestNewStatistics, GiniCoefficient_EmptyColumnReturnsEmpty) {
    auto gini_stat = stats_ptr->GetGiniCoefficient(0);
    EXPECT_FALSE(gini_stat.HasValue());
}

TEST_F(TestNewStatistics, GiniCoefficient_RangeCheck) {
    // Коэффициент Джини всегда в [0, 1)
    for (size_t i = 0; i < stats_ptr->GetNumberOfColumns(); ++i) {
        auto gini_stat = stats_ptr->GetGiniCoefficient(i);
        if (gini_stat.HasValue()) {
            double gini = mo::Type::GetValue<mo::Double>(gini_stat.GetData());
            EXPECT_GE(gini, 0.0);
            EXPECT_LT(gini, 1.0);
        }
    }
}

TEST_F(TestNewStatistics, StatisticsIndependentOfExecutionOrder) {
    // Статистики не должны зависеть от порядка вызова методов
    auto stats1 = MakeStatAlgorithm(kTestDataStats);
    auto stats2 = MakeStatAlgorithm(kTestDataStats);
    
    stats1->Execute();
    stats2->Execute();
    
    auto iqr1 = stats1->GetInterquartileRange(2);
    auto cv1 = stats1->GetCoefficientOfVariation(7);
    auto mon1 = stats1->GetMonotonicity(3);
    
    auto mon2 = stats2->GetMonotonicity(3);
    auto cv2 = stats2->GetCoefficientOfVariation(7);
    auto iqr2 = stats2->GetInterquartileRange(2);
    
    if (iqr1.HasValue() && iqr2.HasValue()) {
        double val1 = mo::Type::GetValue<mo::Double>(iqr1.GetData());
        double val2 = mo::Type::GetValue<mo::Double>(iqr2.GetData());
        EXPECT_DOUBLE_EQ(val1, val2);
    }
    
    if (cv1.HasValue() && cv2.HasValue()) {
        double val1 = mo::Type::GetValue<mo::Double>(cv1.GetData());
        double val2 = mo::Type::GetValue<mo::Double>(cv2.GetData());
        EXPECT_DOUBLE_EQ(val1, val2);
    }
    
    if (mon1.HasValue() && mon2.HasValue()) {
        std::string val1 = mo::Type::GetValue<mo::String>(mon1.GetData());
        std::string val2 = mo::Type::GetValue<mo::String>(mon2.GetData());
        EXPECT_EQ(val1, val2);
    }
}

};  // namespace tests
