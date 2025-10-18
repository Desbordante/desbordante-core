#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "algebraic_constraints/ac_exception.h"
#include "algebraic_constraints/ranges_collection.h"
#include "algebraic_constraints/typed_column_pair.h"
#include "algorithms/algebraic_constraints/ac_algorithm.h"
#include "algorithms/algebraic_constraints/bin_operation_enum.h"
#include "algorithms/algo_factory.h"
#include "all_csv_configs.h"
#include "builtin.h"
#include "config/names.h"
#include "double_type.h"
#include "numeric_type.h"

struct CSVConfig;

namespace {
void AssertRanges(std::vector<model::Double>& expected_ranges,
                  algos::RangesCollection const& byte_ranges) {
    ASSERT_EQ(expected_ranges.size(), byte_ranges.ranges.size());

    model::DoubleType double_type;
    for (size_t i = 0; i < expected_ranges.size(); ++i) {
        auto expected = std::unique_ptr<std::byte[]>(double_type.MakeValue(expected_ranges[i]));
        double_type.CastTo(expected.get(), byte_ranges.col_pair.num_type->GetTypeId());
        EXPECT_EQ(byte_ranges.col_pair.num_type->Compare(expected.get(), byte_ranges.ranges[i]),
                  model::CompareResult::kEqual);
    }
}

void AssertACExceptions(std::vector<algos::ACException>& expected_ac_exceptions,
                        std::vector<algos::ACException> const& actual_ac_exceptions) {
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
    using ACExceptions = std::vector<algos::ACException>;

    static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config, algos::Binop bin_operation,
                                           double fuzziness, double p_fuzz, double weight,
                                           size_t bumps_limit, size_t iterations_limit,
                                           double seed) {
        using namespace config::names;
        return {{kCsvConfig, csv_config},
                {kBinaryOperation, bin_operation},
                {kFuzziness, fuzziness},
                {kFuzzinessProbability, p_fuzz},
                {kWeight, weight},
                {kBumpsLimit, bumps_limit},
                {kIterationsLimit, iterations_limit},
                {kACSeed, seed}};
    }

    static std::unique_ptr<algos::ACAlgorithm> CreateACAlgorithmInstance(
            CSVConfig const& csv_config, algos::Binop bin_operation = algos::Binop::Addition,
            double fuzziness = 0.1, double p_fuzz = 0.9, double weight = 0.1,
            size_t bumps_limit = 0, size_t iterations_limit = 10, double seed = 0) {
        return algos::CreateAndLoadAlgorithm<algos::ACAlgorithm>(
                GetParamMap(csv_config, bin_operation, fuzziness, p_fuzz, weight, bumps_limit,
                            iterations_limit, seed));
    }
};

TEST_F(ACAlgorithmTest, NonFuzzyBumpsDetection1) {
    auto a = CreateACAlgorithmInstance(kIris, algos::Binop::Addition, 0.0, 0.9, 0.05);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(0, 2);

    std::vector<model::Double> expected_ranges = {5.4, 7.4, 8.1, 8.5, 9.1, 14.6};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, NonFuzzyBumpsDetection2) {
    auto a = CreateACAlgorithmInstance(kIris, algos::Binop::Addition, 0.0, 0.9, 0.05);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(2, 3);

    std::vector<model::Double> expected_ranges = {1.2, 2.3, 4.1, 9.2};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, SampleSizeCalculation) {
    auto a = CreateACAlgorithmInstance(kIris, algos::Binop::Addition, 0.1, 0.8, 0.05);
    ASSERT_EQ(28, a->CalculateSampleSize(1));
    /* Sample size can't be greather than number of rows in the table */
    ASSERT_EQ(150, a->CalculateSampleSize(13));
    ASSERT_EQ(150, a->CalculateSampleSize(28));
}

TEST_F(ACAlgorithmTest, SubNonFuzzy) {
    auto a = CreateACAlgorithmInstance(kIris, algos::Binop::Subtraction, 0.0);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(1, 3);

    std::vector<model::Double> expected_ranges = {0.3, 2.0, 2.7, 4.0};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, MulNonFuzzy) {
    auto a = CreateACAlgorithmInstance(kIris, algos::Binop::Multiplication, 0.0);
    a->Execute();
    auto& ranges_collection = a->GetRangesByColumns(2, 3);

    std::vector<model::Double> expected_ranges = {0.11, 0.96, 3.3, 15.87};

    AssertRanges(expected_ranges, ranges_collection);
}

TEST_F(ACAlgorithmTest, DivNonFuzzy) {
    auto a = CreateACAlgorithmInstance(kTestZeros, algos::Binop::Division, 0.0);
    a->Execute();
    auto& ranges_collection01 = a->GetRangesByColumns(0, 1);
    auto& ranges_collection10 = a->GetRangesByColumns(1, 0);

    std::vector<model::Double> expected_ranges01 = {0, 1, 10, 10};
    std::vector<model::Double> true_ranges10 = {0, 0, 1, 1};

    AssertRanges(expected_ranges01, ranges_collection01);
    AssertRanges(true_ranges10, ranges_collection10);

    auto& ranges_collection02 = a->GetRangesByColumns(0, 2);
    auto& ranges_collection20 = a->GetRangesByColumns(2, 0);

    std::vector<model::Double> expected_ranges02 = {1, 1};
    std::vector<model::Double> expected_ranges20 = {0, 0, 1, 1};

    AssertRanges(expected_ranges02, ranges_collection02);
    AssertRanges(expected_ranges20, ranges_collection20);
}

TEST_F(ACAlgorithmTest, FuzzyBumpsDetection) {
    auto a = CreateACAlgorithmInstance(kTestLong, algos::Binop::Addition, 0.55, 0.41, 0.1);
    a->Execute();
    auto& ranges_collection01 = a->GetRangesByColumns(0, 1);
    auto& ranges_collection02 = a->GetRangesByColumns(0, 2);
    auto& ranges_collection12 = a->GetRangesByColumns(1, 2);

    std::vector<model::Double> expected_ranges01 = {3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8};
    std::vector<model::Double> expected_ranges02 = {2, 2, 8, 9, 12, 13};
    std::vector<model::Double> expected_ranges12 = {9, 9, 11, 11};

    AssertRanges(expected_ranges01, ranges_collection01);
    AssertRanges(expected_ranges02, ranges_collection02);
    AssertRanges(expected_ranges12, ranges_collection12);
}

TEST_F(ACAlgorithmTest, NullAndEmptyIgnoring) {
    auto a = CreateACAlgorithmInstance(kNullEmpty, algos::Binop::Addition, 0.0);
    a->Execute();
    auto& ranges = a->GetRangesCollections();
    EXPECT_EQ(ranges.size(), 6);
    auto& ranges_collection01 = a->GetRangesByColumns(0, 1);
    auto& ranges_collection02 = a->GetRangesByColumns(0, 2);
    auto& ranges_collection12 = a->GetRangesByColumns(0, 3);

    std::vector<model::Double> expected_ranges01 = {3, 3};
    std::vector<model::Double> expected_ranges02 = {4, 4};
    std::vector<model::Double> expected_ranges03 = {2, 2};

    AssertRanges(expected_ranges01, ranges_collection01);
    AssertRanges(expected_ranges02, ranges_collection02);
    AssertRanges(expected_ranges03, ranges_collection12);
}

TEST_F(ACAlgorithmTest, ColumnTypesPairing) {
    auto a = CreateACAlgorithmInstance(kSimpleTypes, algos::Binop::Addition, 0.0);
    a->Execute();
    auto& ranges = a->GetRangesCollections();
    EXPECT_EQ(ranges.size(), 2);
}

TEST_F(ACAlgorithmTest, CollectingACExceptions) {
    auto a = CreateACAlgorithmInstance(kTestLong, algos::Binop::Addition, 0.55, 0.41, 0.1);
    a->Execute();
    a->CollectACExceptions();

    algos::ACException e0(0, {{1, 2}});
    algos::ACException e1(1, {{0, 2}, {1, 2}});
    algos::ACException e2(2, {{0, 2}, {1, 2}});
    algos::ACException e3(3, {{0, 2}, {1, 2}});
    ACAlgorithmTest::ACExceptions expected = {e0, e1, e2, e3};

    AssertACExceptions(expected, a->GetACExceptions());
}

TEST_F(ACAlgorithmTest, RangesReconstruction) {
    auto a = CreateACAlgorithmInstance(kIris, algos::Binop::Subtraction, 0.0);
    a->Execute();
    auto ranges_collection = a->ReconstructRangesByColumns(1, 3, 1);
    std::vector<model::Double> expected_ranges = {0.3, 4.0};

    AssertRanges(expected_ranges, ranges_collection);
}
}  // namespace tests
