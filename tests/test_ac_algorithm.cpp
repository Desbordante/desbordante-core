#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "ac_algorithm.h"
#include "datasets.h"
#include "types.h"

namespace {
void AssertRanges(std::vector<std::string>& expected_ranges,
                  algos::RangesCollection const& byte_ranges) {
    ASSERT_EQ(expected_ranges.size(), byte_ranges.ranges.size());

    auto expected = std::unique_ptr<std::byte[]>(byte_ranges.col_pair.num_type->Allocate());
    for (size_t i = 0; i < expected_ranges.size(); ++i) {
        // FIXME:
        /* Для значения "7.4": сравнение через num_type->Compare() значений в виде std::byte
         * (полученных с помощью num_type->ValueFromStr()) возвращает, что они не равны,
         * однако при использовании на этих же значениях num_type->ValueToString()
         * полученные строки оказываются равными */
        byte_ranges.col_pair.num_type->ValueFromStr(expected.get(), expected_ranges[i]);
        EXPECT_EQ(byte_ranges.col_pair.num_type->ValueToString(expected.get()),
                  byte_ranges.col_pair.num_type->ValueToString(byte_ranges.ranges[i]));
    }
}

void AssertACExceptions(
        std::vector<algos::ACExceptionFinder::ACException>& expected_ac_exceptions,
        std::vector<algos::ACExceptionFinder::ACException> const& actual_ac_exceptions) {
    ASSERT_EQ(expected_ac_exceptions.size(), actual_ac_exceptions.size());

    for (size_t i = 0; i < expected_ac_exceptions.size(); ++i) {
        ASSERT_EQ(expected_ac_exceptions[i].row_i, actual_ac_exceptions[i].row_i);
        size_t pairs_amount = expected_ac_exceptions[i].column_pairs.size();
        ASSERT_EQ(pairs_amount, actual_ac_exceptions[i].column_pairs.size());
        for (size_t pair_i = 0; pair_i < pairs_amount; ++pair_i) {
            EXPECT_EQ(expected_ac_exceptions[i].column_pairs[pair_i],
                      actual_ac_exceptions[i].column_pairs[pair_i]);
        }
    }
}
}  // namespace

namespace tests {

namespace fs = std::filesystem;

class ACAlgorithmTest : public ::testing::Test {
public:
    using ACExceptions = std::vector<algos::ACExceptionFinder::ACException>;

    static std::unique_ptr<algos::ACAlgorithm> CreateACAlgorithmInstance(
            std::string_view path, char separator = ',', bool hasHeader = true,
            char bin_operation = '+', double fuzziness = 0.1, double p_fuzz = 0.9,
            double weight = 0.1, size_t bumps_limit = 0, size_t iterations_limit = 10,
            std::string pairing_rule = "trivial", double seed = 0) {
        auto data = test_data_dir / path;
        algos::ACAlgorithm::Config const config = {
                data,   separator,   hasHeader,        bin_operation, fuzziness, p_fuzz,
                weight, bumps_limit, iterations_limit, pairing_rule,  seed};
        return std::make_unique<algos::ACAlgorithm>(config);
    }
};

TEST_F(ACAlgorithmTest, NonFuzzyBumpsDetection1) {
    auto a = CreateACAlgorithmInstance("iris.csv", ',', false, '+', 0.0, 1, 0.05);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(0, 2);

    std::vector<std::string> expected_ranges = {"5.4", "7.4", "8.1", "8.5", "9.1", "14.6"};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, NonFuzzyBumpsDetection2) {
    auto a = CreateACAlgorithmInstance("iris.csv", ',', false, '+', 0.0, 1, 0.05);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(2, 3);

    std::vector<std::string> expected_ranges = {"1.2", "2.3", "4.1", "9.2"};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, SampleSizeCalculation) {
    auto a = CreateACAlgorithmInstance("iris.csv", ',', false, '+', 0.1, 0.8, 0.05);
    ASSERT_EQ(28, a->CalculateSampleSize(1));
    ASSERT_EQ(168, a->CalculateSampleSize(13));
    ASSERT_EQ(331, a->CalculateSampleSize(28));
}

TEST_F(ACAlgorithmTest, SubNonFuzzy) {
    auto a = CreateACAlgorithmInstance("iris.csv", ',', false, '-', 0.0, 1, 0.1);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(1, 3);

    std::vector<std::string> expected_ranges = {"0.3", "2.0", "2.7", "4.0"};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, MulNonFuzzy) {
    auto a = CreateACAlgorithmInstance("iris.csv", ',', false, '*', 0.0, 1, 0.1);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(2, 3);

    std::vector<std::string> expected_ranges = {"0.11", "0.96", "3.3", "15.87"};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, DivNonFuzzy) {
    auto a = CreateACAlgorithmInstance("TestZeros.csv", ',', true, '/', 0.0, 1, 0.1);
    a->Execute();
    auto& ranges_collection01 = a->GetRangesByColumns(0, 1);
    auto& ranges_collection10 = a->GetRangesByColumns(1, 0);

    std::vector<std::string> expected_ranges01 = {"0", "1", "10", "10"};
    std::vector<std::string> true_ranges10 = {"0", "0", "1", "1"};

    AssertRanges(expected_ranges01, ranges_collection01);
    AssertRanges(true_ranges10, ranges_collection10);

    auto& ranges_collection02 = a->GetRangesByColumns(0, 2);
    auto& ranges_collection20 = a->GetRangesByColumns(2, 0);

    std::vector<std::string> expected_ranges02 = {"1", "1"};
    std::vector<std::string> expected_ranges20 = {"0", "0", "1", "1"};

    AssertRanges(expected_ranges02, ranges_collection02);
    AssertRanges(expected_ranges20, ranges_collection20);
}

TEST_F(ACAlgorithmTest, FuzzyBumpsDetection) {
    auto a = CreateACAlgorithmInstance("TestLong.csv", ',', true, '+', 0.55, 0.41, 0.1);
    a->Execute();
    auto& ranges_collection01 = a->GetRangesByColumns(0, 1);
    auto& ranges_collection02 = a->GetRangesByColumns(0, 2);
    auto& ranges_collection12 = a->GetRangesByColumns(1, 2);

    std::vector<std::string> expected_ranges01 = {"3", "3", "4", "4", "5", "5",
                                                  "6", "6", "7", "7", "8", "8"};
    std::vector<std::string> expected_ranges02 = {"2", "2", "8", "9", "12", "13"};
    std::vector<std::string> expected_ranges12 = {"9", "9", "11", "11"};

    AssertRanges(expected_ranges01, ranges_collection01);
    AssertRanges(expected_ranges02, ranges_collection02);
    AssertRanges(expected_ranges12, ranges_collection12);
}

TEST_F(ACAlgorithmTest, NullAndEmptyIgnoring) {
    auto a = CreateACAlgorithmInstance("NullEmpty.csv", ',', true, '+', 0.0, 1, 0.1);
    a->Execute();
    auto& ranges = a->GetRangesCollections();
    EXPECT_EQ(ranges.size(), 6);
    auto& ranges_collection01 = a->GetRangesByColumns(0, 1);
    auto& ranges_collection02 = a->GetRangesByColumns(0, 2);
    auto& ranges_collection12 = a->GetRangesByColumns(0, 3);

    std::vector<std::string> expected_ranges01 = {"3", "3"};
    std::vector<std::string> expected_ranges02 = {"4", "4"};
    std::vector<std::string> expected_ranges03 = {"2", "2"};

    AssertRanges(expected_ranges01, ranges_collection01);
    AssertRanges(expected_ranges02, ranges_collection02);
    AssertRanges(expected_ranges03, ranges_collection12);
}

TEST_F(ACAlgorithmTest, ColumnTypesPairing) {
    auto a = CreateACAlgorithmInstance("SimpleTypes.csv", ',', true, '+', 0.0, 1, 0.1);
    a->Execute();
    auto& ranges = a->GetRangesCollections();
    EXPECT_EQ(ranges.size(), 1);
}

TEST_F(ACAlgorithmTest, CollectingACExceptions) {
    auto a = CreateACAlgorithmInstance("TestLong.csv", ',', true, '+', 0.55, 0.41, 0.1);
    a->Execute();
    a->CollectACExceptions();

    algos::ACExceptionFinder::ACException e0(0, {{1, 2}});
    algos::ACExceptionFinder::ACException e1(1, {{0, 2}, {1, 2}});
    algos::ACExceptionFinder::ACException e2(2, {{0, 2}, {1, 2}});
    algos::ACExceptionFinder::ACException e3(3, {{0, 2}, {1, 2}});
    ACAlgorithmTest::ACExceptions expected = {e0, e1, e2, e3};

    AssertACExceptions(expected, a->GetACExceptions());
}

TEST_F(ACAlgorithmTest, RangesReconstruction) {
    auto a = CreateACAlgorithmInstance("iris.csv", ',', false, '-', 0.0, 1, 0.1);
    a->Execute();
    auto ranges_collection = a->ReconstructRangesByColumns(1, 3, 1);
    std::vector<std::string> expected_ranges = {"0.3", "4.0"};

    AssertRanges(expected_ranges, ranges_collection);
}
}  // namespace tests
