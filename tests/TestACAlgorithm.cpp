#include <filesystem>
#include <string>

#include "ACAlgorithm.h"
#include "Types.h"
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
            path,    separator,    hasHeader,        bin_operation_, fuzziness_, p_fuzz_,
            weight_, bumps_limit_, iterations_limit, pairing_rule_,  test_mode};
        return std::make_unique<algos::ACAlgorithm>(config);
    }
};

void AssertRanges(std::vector<std::string>& true_ranges,
                  const algos::ACAlgorithm::RangesCollection& byte_ranges) {
    ASSERT_EQ(true_ranges.size(), byte_ranges.ranges.size());

    model::INumericType* num_type = dynamic_cast<model::INumericType*>(byte_ranges.num_type.get());

    for (size_t i = 0; i < true_ranges.size(); ++i) {
        if (true_ranges[i] != num_type->ValueToString(byte_ranges.ranges[i])) {
            FAIL();
        }
    }

    SUCCEED();
}

TEST_F(ACAlgorithmTest, NonFuzzyBumpsDetection1) {
    std::filesystem::path path("inputData/iris.csv");
    auto a = CreateACAlgorithmInstance(path, ',', false, '+', 0.0, 1, 0.05);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(0, 2);

    std::vector<std::string> true_ranges = {"5.400000", "7.400000", "8.100000",
                                            "8.500000", "9.100000", "14.600000"};

    AssertRanges(true_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, NonFuzzyBumpsDetection2) {
    std::filesystem::path path("inputData/iris.csv");
    auto a = CreateACAlgorithmInstance(path, ',', false, '+', 0.0, 1, 0.05);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(2, 3);

    std::vector<std::string> true_ranges = {"1.200000", "2.300000", "4.100000", "9.200000"};

    AssertRanges(true_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, SampleSizeCalculation) {
    std::filesystem::path path("inputData/iris.csv");
    auto a = CreateACAlgorithmInstance(path, ',', false, '+', 0.1, 0.8, 0.05);
    ASSERT_EQ(28, a->CalculateSampleSize(1));
    ASSERT_EQ(168, a->CalculateSampleSize(13));
    ASSERT_EQ(331, a->CalculateSampleSize(28));
}

TEST_F(ACAlgorithmTest, SubNonFuzzy) {
    std::filesystem::path path("inputData/iris.csv");
    auto a = CreateACAlgorithmInstance(path, ',', false, '-', 0.0, 1, 0.1);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(1, 3);

    std::vector<std::string> true_ranges = {"0.300000", "2.000000", "2.700000", "4.000000"};

    AssertRanges(true_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, MulNonFuzzy) {
    std::filesystem::path path("inputData/iris.csv");
    auto a = CreateACAlgorithmInstance(path, ',', false, '*', 0.0, 1, 0.1);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(2, 3);

    std::vector<std::string> true_ranges = {"0.110000", "0.960000", "3.300000", "15.870000"};

    AssertRanges(true_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, DivNonFuzzy) {
    std::filesystem::path path("../../tests/inputData/TestZeros.csv");
    auto a = CreateACAlgorithmInstance(path, ',', true, '/', 0.0, 1, 0.1);
    a->Execute();
    auto& ranges_collection01 = a->GetRangesByColumns(0, 1);
    auto& ranges_collection10 = a->GetRangesByColumns(1, 0);

    std::vector<std::string> true_ranges01 = {"0", "1", "10", "10"};
    std::vector<std::string> true_ranges10 = {"0", "0", "1", "1"};

    AssertRanges(true_ranges01, ranges_collection01);
    AssertRanges(true_ranges10, ranges_collection10);

    auto& ranges_collection02 = a->GetRangesByColumns(0, 2);
    auto& ranges_collection20 = a->GetRangesByColumns(2, 0);

    std::vector<std::string> true_ranges02 = {"1", "1"};
    std::vector<std::string> true_ranges20 = {"0", "0", "1", "1"};

    AssertRanges(true_ranges02, ranges_collection02);
    AssertRanges(true_ranges20, ranges_collection20);
}

TEST_F(ACAlgorithmTest, FuzzyBumpsDetection) {
    std::filesystem::path path("inputData/TestLong.csv");
    auto a = CreateACAlgorithmInstance(path, ',', true, '+', 0.55, 0.41, 0.1);
    a->Execute();
    auto& ranges_collection01 = a->GetRangesByColumns(0, 1);
    auto& ranges_collection02 = a->GetRangesByColumns(0, 2);
    auto& ranges_collection12 = a->GetRangesByColumns(1, 2);

    std::vector<std::string> true_ranges01 = {"3", "3", "4", "4", "5", "5",
                                              "6", "6", "7", "7", "8", "8"};
    std::vector<std::string> true_ranges02 = {"2", "2", "8", "9", "12", "13"};
    std::vector<std::string> true_ranges12 = {"9", "9", "11", "11"};

    AssertRanges(true_ranges01, ranges_collection01);
    AssertRanges(true_ranges02, ranges_collection02);
    AssertRanges(true_ranges12, ranges_collection12);
}
}  // namespace tests