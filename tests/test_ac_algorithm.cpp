#include <filesystem>
#include <string>

#include "ac_algorithm.h"
#include "types.h"
#include "gtest/gtest.h"

namespace fs = std::filesystem;

namespace tests {

class ACAlgorithmTest : public ::testing::Test {
public:
    static std::unique_ptr<algos::ACAlgorithm> CreateACAlgorithmInstance(
        std::filesystem::path const& path, char separator = ',', bool hasHeader = true,
        char bin_operation_ = '+', double fuzziness_ = 0.1, double p_fuzz_ = 0.9,
        double weight_ = 0.1, size_t bumps_limit_ = 0, size_t iterations_limit = 10,
        std::string pairing_rule_ = "trivial", bool test_mode = true) {
        algos::ACAlgorithm::Config const config = {
            path,    separator, hasHeader,    bin_operation_,   fuzziness_,
            p_fuzz_, weight_,   bumps_limit_, iterations_limit, pairing_rule_};
        return std::make_unique<algos::ACAlgorithm>(config, test_mode);
    }
};

void AssertRanges(std::vector<std::string>& expected_ranges,
                  const algos::ACAlgorithm::RangesCollection& byte_ranges) {
    ASSERT_EQ(expected_ranges.size(), byte_ranges.ranges.size());

    auto expected = std::unique_ptr<std::byte[]>(new std::byte[byte_ranges.num_type->GetSize()]);
    for (size_t i = 0; i < expected_ranges.size(); ++i) {
        // find out why ValueFromStr() gives incorrect value
        byte_ranges.num_type->ValueFromStr(expected.get(), expected_ranges[i]);
        EXPECT_EQ(byte_ranges.num_type->ValueToString(expected.get()),
                  byte_ranges.num_type->ValueToString(byte_ranges.ranges[i]));
    }
}

TEST_F(ACAlgorithmTest, NonFuzzyBumpsDetection1) {
    auto path = std::filesystem::current_path() / "input_data" / "iris.csv";
    auto a = CreateACAlgorithmInstance(path, ',', false, '+', 0.0, 1, 0.05);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(0, 2);

    std::vector<std::string> expected_ranges = {"5.4", "7.4", "8.1", "8.5", "9.1", "14.6"};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, NonFuzzyBumpsDetection2) {
    auto path = std::filesystem::current_path() / "input_data" / "iris.csv";
    auto a = CreateACAlgorithmInstance(path, ',', false, '+', 0.0, 1, 0.05);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(2, 3);

    std::vector<std::string> expected_ranges = {"1.2", "2.3", "4.1", "9.2"};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, SampleSizeCalculation) {
    auto path = std::filesystem::current_path() / "input_data" / "iris.csv";
    auto a = CreateACAlgorithmInstance(path, ',', false, '+', 0.1, 0.8, 0.05);
    ASSERT_EQ(28, a->CalculateSampleSize(1));
    ASSERT_EQ(168, a->CalculateSampleSize(13));
    ASSERT_EQ(331, a->CalculateSampleSize(28));
}

TEST_F(ACAlgorithmTest, SubNonFuzzy) {
    auto path = std::filesystem::current_path() / "input_data" / "iris.csv";
    auto a = CreateACAlgorithmInstance(path, ',', false, '-', 0.0, 1, 0.1);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(1, 3);

    std::vector<std::string> expected_ranges = {"0.3", "2.0", "2.7", "4.0"};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, MulNonFuzzy) {
    auto path = std::filesystem::current_path() / "input_data" / "iris.csv";
    auto a = CreateACAlgorithmInstance(path, ',', false, '*', 0.0, 1, 0.1);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(2, 3);

    std::vector<std::string> expected_ranges = {"0.11", "0.96", "3.3", "15.87"};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, DivNonFuzzy) {
    auto path = std::filesystem::current_path() / "input_data" / "TestZeros.csv";
    auto a = CreateACAlgorithmInstance(path, ',', true, '/', 0.0, 1, 0.1);
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
    auto path = std::filesystem::current_path() / "input_data" / "TestLong.csv";
    auto a = CreateACAlgorithmInstance(path, ',', true, '+', 0.55, 0.41, 0.1);
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
    auto path = std::filesystem::current_path() / "input_data" / "NullEmpty.csv";
    auto a = CreateACAlgorithmInstance(path, ',', true, '+', 0.0, 1, 0.1);
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
    auto path = std::filesystem::current_path() / "input_data" / "SimpleTypes.csv";
    auto a = CreateACAlgorithmInstance(path, ',', true, '+', 0.0, 1, 0.1);
    a->Execute();
    auto& ranges = a->GetRangesCollections();
    EXPECT_EQ(ranges.size(), 1);
}
}  // namespace tests
