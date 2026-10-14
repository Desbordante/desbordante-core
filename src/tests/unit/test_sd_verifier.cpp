#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/sd/sd_verifier/sd_verifier.h"
#include "core/algorithms/sd/util/numeric_utils.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace tests {

struct SDVerifierParams {
    std::string name;
    std::string active_csv_filename;
    algos::StdParamsMap params;
    long expected_ops;
    double expected_confidence;
    bool check_violations;
    std::vector<size_t> expected_deletions;
    std::vector<std::pair<size_t, size_t>> expected_insertions;
    std::optional<config::IndicesType> indices_opt;
    std::optional<std::string> custom_csv_content;
    bool expect_exception;

    SDVerifierParams(std::string test_name, config::IndicesType lhs, config::IndicesType rhs,
                     double g1, double g2, long ops, double conf, bool check_violations = false,
                     std::vector<size_t> deletions = {},
                     std::vector<std::pair<size_t, size_t>> insertions = {},
                     std::optional<config::IndicesType> indices = std::nullopt,
                     std::optional<std::string> csv_filename = std::nullopt,
                     std::optional<std::string> csv_content = std::nullopt,
                     bool expect_exception = false)
        : name(std::move(test_name)),
          active_csv_filename(csv_filename.value_or("sd_test_gen_" + name)),
          params({{config::names::kCsvConfig, CSVConfig{active_csv_filename.c_str(), ',', true}},
                  {config::names::kLhsIndices, lhs},
                  {config::names::kRhsIndices, rhs},
                  {config::names::kSdG1, g1},
                  {config::names::kSdG2, g2}}),
          expected_ops(ops),
          expected_confidence(conf),
          check_violations(check_violations),
          expected_deletions(std::move(deletions)),
          expected_insertions(std::move(insertions)),
          indices_opt(indices),
          custom_csv_content(csv_content),
          expect_exception(expect_exception) {}
};

struct ScopedTestFile {
    std::string filename;

    ScopedTestFile(std::string const& name, std::string const& content) : filename(name) {
        std::ofstream f(filename);
        f << content;
    }

    ~ScopedTestFile() {
        std::remove(filename.c_str());
    }
};

long BruteForceOps(std::vector<double> const& values, double g1, double g2) {
    size_t const n = values.size();
    if (n == 0) {
        return 0;
    }

    long best = static_cast<long>(n);
    size_t const subsequences = size_t{1} << n;
    for (size_t mask = 1; mask < subsequences; ++mask) {
        long cost = static_cast<long>(n);
        std::optional<size_t> previous;
        bool valid = true;

        for (size_t i = 0; i < n; ++i) {
            if ((mask & (size_t{1} << i)) == 0) {
                continue;
            }
            --cost;

            if (previous.has_value()) {
                double const distance = values[i] - values[*previous];
                if (distance < g1) {
                    valid = false;
                    break;
                }

                long steps = 1;
                if (g2 >= 0.0) {
                    steps = std::max(1L, static_cast<long>(std::ceil(distance / g2)));
                    if (static_cast<double>(steps) * g1 > distance) {
                        valid = false;
                        break;
                    }
                }
                cost += steps - 1;
            }
            previous = i;
        }

        if (valid) {
            best = std::min(best, cost);
        }
    }
    return best;
}

long BruteForceOpsUsingCalculator(std::vector<double> const& values,
                                  algos::sd::util::EditDistanceCalculator const& calculator) {
    size_t const n = values.size();
    if (n == 0) {
        return 0;
    }

    long best = static_cast<long>(n);
    size_t const subsequences = size_t{1} << n;
    for (size_t mask = 1; mask < subsequences; ++mask) {
        long cost = static_cast<long>(n);
        std::optional<size_t> previous;
        bool valid = true;

        for (size_t i = 0; i < n; ++i) {
            if ((mask & (size_t{1} << i)) == 0) {
                continue;
            }
            --cost;

            if (previous.has_value()) {
                long const steps = calculator.CalculateDCost(values[i] - values[*previous]);
                if (steps < 1) {
                    valid = false;
                    break;
                }
                cost += steps - 1;
            }
            previous = i;
        }

        if (valid) {
            best = std::min(best, cost);
        }
    }
    return best;
}

class SDVerifierTest : public ::testing::TestWithParam<SDVerifierParams> {};

TEST_P(SDVerifierTest, ValidationResult) {
    auto const& p = GetParam();

    std::optional<ScopedTestFile> scoped_file;
    if (p.custom_csv_content.has_value()) {
        scoped_file.emplace(p.active_csv_filename, *p.custom_csv_content);
    }

    if (p.expect_exception) {
        EXPECT_THROW(
                {
                    auto verifier =
                            algos::CreateAndLoadAlgorithm<algos::sd_verifier::SDVerifier>(p.params);
                    if (p.indices_opt.has_value()) {
                        verifier->SetOption(config::names::kSdIndices, *p.indices_opt);
                    }
                    verifier->Execute();
                },
                std::runtime_error);
        return;
    }

    auto verifier = algos::CreateAndLoadAlgorithm<algos::sd_verifier::SDVerifier>(p.params);

    if (p.indices_opt.has_value()) {
        verifier->SetOption(config::names::kSdIndices, *p.indices_opt);
    }

    verifier->Execute();

    EXPECT_EQ(verifier->GetOPS(), p.expected_ops);
    EXPECT_NEAR(verifier->GetConfidence(), p.expected_confidence, 1e-5);

    if (p.check_violations) {
        auto const& violations = verifier->GetViolations();

        std::vector<size_t> actual_deletions;
        std::vector<std::pair<size_t, size_t>> actual_insertions;
        long reconstructed_ops = 0;
        for (auto const& v : violations) {
            if (std::holds_alternative<algos::sd_verifier::SDInsertion>(v)) {
                auto const& ins = std::get<algos::sd_verifier::SDInsertion>(v);
                actual_insertions.emplace_back(ins.left_row_idx, ins.right_row_idx);
                reconstructed_ops += ins.min_insertions;
            } else {
                auto const& del = std::get<algos::sd_verifier::SDDeletion>(v);
                actual_deletions.emplace_back(del.row_idx);
                ++reconstructed_ops;
            }
        }
        std::sort(actual_insertions.begin(), actual_insertions.end());
        std::sort(actual_deletions.begin(), actual_deletions.end());

        EXPECT_EQ(actual_deletions, p.expected_deletions);
        EXPECT_EQ(actual_insertions, p.expected_insertions);
        EXPECT_EQ(reconstructed_ops, verifier->GetOPS());
    }
}

TEST(SDNumericUtilsTest, ParsesFiniteNumbersWithSurroundingWhitespace) {
    EXPECT_DOUBLE_EQ(algos::sd::util::ParseNumeric(" \t1.25\n"), 1.25);
    EXPECT_DOUBLE_EQ(algos::sd::util::ParseNumeric("-2.5e1"), -25.0);
    EXPECT_DOUBLE_EQ(algos::sd::util::ParseNumeric("+0"), 0.0);
}

TEST(SDNumericUtilsTest, RejectsInvalidOrNonFiniteNumbers) {
    for (auto const* value : {"", " ", "1foo", "1 2", "nan", "inf", "-inf", "1e309"}) {
        SCOPED_TRACE(value);
        EXPECT_THROW((void)algos::sd::util::ParseNumeric(value), std::runtime_error);
    }
}

TEST(SDEditDistanceCalculatorTest, MatchesExhaustiveSmallIntegerOracle) {
    std::array<double, 4> constexpr values_domain{-1.0, 0.0, 1.0, 2.0};
    std::array<std::pair<double, double>, 5> constexpr gaps{
            std::pair{0.0, -1.0}, std::pair{0.0, 2.0}, std::pair{1.0, 2.0}, std::pair{1.0, 1.0},
            std::pair{2.0, 3.0}};

    for (size_t length = 0; length <= 5; ++length) {
        size_t sequence_count = 1;
        for (size_t i = 0; i < length; ++i) {
            sequence_count *= values_domain.size();
        }

        for (size_t encoded = 0; encoded < sequence_count; ++encoded) {
            std::vector<double> values(length);
            size_t remaining = encoded;
            for (double& value : values) {
                value = values_domain[remaining % values_domain.size()];
                remaining /= values_domain.size();
            }

            for (auto const& [g1, g2] : gaps) {
                algos::sd::util::EditDistanceCalculator calculator(g1, g2);
                long const expected = BruteForceOps(values, g1, g2);
                EXPECT_EQ(calculator.CalculateTrace(values).ops, expected)
                        << "g1=" << g1 << ", g2=" << g2
                        << ", values=" << ::testing::PrintToString(values);

                auto const prefix_ops = calculator.CalculatePrefixOps(values);
                ASSERT_EQ(prefix_ops.size(), values.size());
                for (size_t prefix_length = 1; prefix_length <= values.size(); ++prefix_length) {
                    std::vector<double> const prefix(values.begin(),
                                                     values.begin() + prefix_length);
                    EXPECT_EQ(prefix_ops[prefix_length - 1], BruteForceOps(prefix, g1, g2))
                            << "g1=" << g1 << ", g2=" << g2
                            << ", prefix=" << ::testing::PrintToString(prefix);
                }
            }
        }
    }
}

TEST(SDEditDistanceCalculatorTest, ExactGapMatchesFractionalFeasibilityOracle) {
    std::array<double, 4> constexpr values_domain{0.0, 0.1, 0.2, 0.200000000000001};
    algos::sd::util::EditDistanceCalculator const calculator(0.1, 0.1);

    for (size_t length = 0; length <= 5; ++length) {
        size_t sequence_count = 1;
        for (size_t i = 0; i < length; ++i) {
            sequence_count *= values_domain.size();
        }

        for (size_t encoded = 0; encoded < sequence_count; ++encoded) {
            std::vector<double> values(length);
            size_t remaining = encoded;
            for (double& value : values) {
                value = values_domain[remaining % values_domain.size()];
                remaining /= values_domain.size();
            }

            EXPECT_EQ(calculator.CalculateTrace(values).ops,
                      BruteForceOpsUsingCalculator(values, calculator))
                    << "values=" << ::testing::PrintToString(values);
        }
    }
}

TEST(SDEditDistanceCalculatorTest, PreservesBestSourceAcrossTiedSegmentTreeKeys) {
    std::vector<double> const values{-1.0, 1.1, 0.2, 3.0, 1.0, 3.0, 0.0, -1e-15};
    algos::sd::util::EditDistanceCalculator const calculator(0.0, 2.0);

    EXPECT_EQ(calculator.CalculateTrace(values).ops, 4);
}

TEST(SDEditDistanceCalculatorTest, ValidatesBestIntervalCandidateNearCeilBoundary) {
    std::vector<double> const values{-0.20000000000001067, -0.049999999999988901,
                                     -0.14999999999999247, -0.049999999999982017};
    algos::sd::util::EditDistanceCalculator const calculator(0.0, 0.1);

    ASSERT_EQ(BruteForceOpsUsingCalculator(values, calculator), 1);
    EXPECT_EQ(calculator.CalculateTrace(values).ops, 1);
}

TEST(SDEditDistanceCalculatorTest, KeepsStrictMinimumAcrossCloseToleranceKeys) {
    std::vector<double> const values{-0.4749999999999876, -0.47500000000000936,
                                     -0.37499999999998312};
    algos::sd::util::EditDistanceCalculator const calculator(0.0, 0.1);

    ASSERT_EQ(BruteForceOpsUsingCalculator(values, calculator), 1);
    EXPECT_EQ(calculator.CalculateTrace(values).ops, 1);
}

TEST(SDEditDistanceCalculatorTest, FindsExactGapPredecessorAcrossModuloWrapBoundary) {
    std::vector<double> const values{-1.2656542480726785e-14, 1.9095836023552692e-14,
                                     1.0150990331349021e-12};
    algos::sd::util::EditDistanceCalculator const calculator(1e-12, 1e-12);

    ASSERT_EQ(BruteForceOpsUsingCalculator(values, calculator), 1);
    EXPECT_EQ(calculator.CalculateTrace(values).ops, 1);
}

TEST(SDEditDistanceCalculatorTest, FindsExactGapPredecessorAcrossRemainderClassThreshold) {
    std::vector<double> const values{0.13000000000001111,   -0.27000000000000002,
                                     0.13000000000002443,   -0.070000000000019991,
                                     0.030000000000011101,  0.029999999999980015,
                                     -0.070000000000024432, 0.13};
    algos::sd::util::EditDistanceCalculator const calculator(0.1, 0.1);

    ASSERT_EQ(BruteForceOpsUsingCalculator(values, calculator), 5);
    EXPECT_EQ(calculator.CalculateTrace(values).ops, 5);
}

TEST(SDVerifierTest, ValidatesConditionalGapAndSubsetOptionsWhenSet) {
    ScopedTestFile file("sd_verifier_option_lifecycle_test.csv", "X,Y\n1,0\n2,1\n");
    auto verifier = std::make_unique<algos::sd_verifier::SDVerifier>();
    algos::LoadAlgorithmData(
            *verifier, {{config::names::kCsvConfig, CSVConfig{file.filename.c_str(), ',', true}}});

    EXPECT_THROW(verifier->SetOption(config::names::kSdG2, 1.0), config::ConfigurationError);
    EXPECT_THROW(verifier->SetOption(config::names::kSdIndices, config::IndicesType{2}),
                 std::runtime_error);
    EXPECT_NO_THROW(verifier->SetOption(config::names::kSdIndices, config::IndicesType{1, 0}));

    verifier->SetOption(config::names::kSdG1, 0.0);
    EXPECT_NO_THROW(verifier->SetOption(config::names::kSdG2, 1.0));
}

INSTANTIATE_TEST_SUITE_P(
        SDVerifierScenarios, SDVerifierTest,
        ::testing::ValuesIn(
                {SDVerifierParams("BasicViolation", {0}, {1}, 0.0, 10.0, 1, 0.888888, true, {},
                                  {std::make_pair(3, 4)}, std::nullopt, std::nullopt,
                                  "X,Y\n1,0\n2,5\n3,10\n4,20\n5,40\n6,45\n7,55\n8,65\n9,70\n"),

                 SDVerifierParams("NegativeG2MeansNoConstraint", {0}, {1}, 0.0, -1.0, 0, 1.0, true,
                                  {}, {}, std::nullopt, std::nullopt,
                                  "X,Y\n1,0\n2,5\n3,10\n4,20\n5,40\n6,45\n7,55\n8,65\n9,70\n"),

                 SDVerifierParams("HighGapWithDeletions", {0}, {1}, 4.0, 5.0, 6, 0.333333, false,
                                  {0, 1, 5, 7}, {}, std::nullopt, std::nullopt,
                                  "X,Y\n1,0\n2,5\n3,10\n4,20\n5,40\n6,45\n7,55\n8,65\n9,70\n"),

                 SDVerifierParams("EqualG1G2Strict", {0}, {1}, 4.0, 4.0, 8, 0.111111, false, {}, {},
                                  std::nullopt, std::nullopt,
                                  "X,Y\n1,0\n2,5\n3,10\n4,20\n5,40\n6,45\n7,55\n8,65\n9,70\n"),

                 SDVerifierParams("IndicesSubset", {0}, {1}, 0.0, 10.0, 0, 1.0, true, {}, {},
                                  config::IndicesType{0, 1, 2}, std::nullopt,
                                  "X,Y\n1,0\n2,5\n3,10\n4,20\n5,40\n6,45\n7,55\n8,65\n9,70\n"),

                 SDVerifierParams("IndicesSubsetWithViolation", {0}, {1}, 0.0, 10.0, 1, 0.5, true,
                                  {}, {std::make_pair(3, 4)}, config::IndicesType{3, 4},
                                  std::nullopt,
                                  "X,Y\n1,0\n2,5\n3,10\n4,20\n5,40\n6,45\n7,55\n8,65\n9,70\n"),

                 SDVerifierParams("VeryHighGap", {0}, {1}, 15.0, 25.0, 4, 0.555555, true,
                                  {0, 2, 5, 7}, {}, std::nullopt, std::nullopt,
                                  "X,Y\n1,0\n2,5\n3,10\n4,20\n5,40\n6,45\n7,55\n8,65\n9,70\n"),

                 SDVerifierParams("MixedDeletionsAndInsertions", {0}, {1}, 10.0, 15.0, 4, 0.555555,
                                  true, {1, 4, 7}, {std::make_pair(3, 5)}, std::nullopt,
                                  std::nullopt,
                                  "X,Y\n1,0\n2,5\n3,10\n4,20\n5,40\n6,45\n7,55\n8,65\n9,70\n"),

                 SDVerifierParams("InvalidGapRangeRejected", {0}, {1}, 5.0, 1.0, 0, 1.0, false, {},
                                  {}, std::nullopt, std::nullopt, "X,Y\n1,0\n2,5\n3,10\n", true),

                 SDVerifierParams("ZeroGapRangeRejected", {0}, {1}, 0.0, 0.0, 0, 1.0, false, {}, {},
                                  std::nullopt, std::nullopt, "X,Y\n1,0\n2,5\n3,10\n", true),

                 SDVerifierParams("NonNumericCellCausesExecutionFailure", {0}, {1}, 0.0, 10.0, 0,
                                  1.0, false, {}, {}, std::nullopt, std::nullopt,
                                  "X,Y\n1,0\n2,abc\n3,10\n", true),

                 SDVerifierParams("SingleRowDataset", {0}, {1}, 0.0, 10.0, 0, 1.0, true, {}, {},
                                  std::nullopt, std::nullopt, "X,Y\n1,10\n"),

                 SDVerifierParams("TwoRowDatasetHolds", {0}, {1}, 0.0, 10.0, 0, 1.0, true, {}, {},
                                  config::IndicesType{0, 1}, std::nullopt, "X,Y\n1,0\n2,5\n"),

                 SDVerifierParams("NegativeYValues", {0}, {1}, 0.0, 10.0, 0, 1.0, true, {}, {},
                                  std::nullopt, std::nullopt,
                                  "X,Y\n1,-10\n2,-5\n3,0\n4,10\n5,20\n"),

                 SDVerifierParams("UnsortedByX", {0}, {1}, 0.0, 10.0, 1, 0.8, true, {},
                                  {std::make_pair(4, 0)}, std::nullopt, std::nullopt,
                                  "X,Y\n5,40\n1,0\n3,10\n2,5\n4,20\n"),

                 SDVerifierParams("AllSameYValues", {0}, {1}, 0.0, 10.0, 0, 1.0, true, {}, {},
                                  std::nullopt, std::nullopt, "X,Y\n1,10\n2,10\n3,10\n4,10\n"),

                 SDVerifierParams("NegativeAndPositiveYValues", {0}, {1}, 0.0, 20.0, 0, 1.0, true,
                                  {}, {}, std::nullopt, std::nullopt,
                                  "X,Y\n1,-20\n2,-10\n3,5\n4,15\n5,25\n"),

                 SDVerifierParams("LargeGapValues", {0}, {1}, 500.0, 1500.0, 0, 1.0, true, {}, {},
                                  std::nullopt, std::nullopt, "X,Y\n1,0\n2,1000\n3,2000\n4,3000\n"),

                 SDVerifierParams("ExactGapG1EqualsG2", {0}, {1}, 10.0, 10.0, 0, 1.0, true, {}, {},
                                  std::nullopt, std::nullopt, "X,Y\n1,-15\n2,-5\n3,5\n4,15\n"),

                 SDVerifierParams("FloatingBoundaryAtMaximumGap", {0}, {1}, 1.0, 2.0, 0, 1.0, false,
                                  {}, {}, std::nullopt, std::nullopt, "X,Y\n1,0.9\n2,2.9\n"),

                 SDVerifierParams("ExactGapNearEqualValues", {0}, {1}, 0.1, 0.1, 2, 1.0 / 3.0,
                                  false, {}, {}, std::nullopt, std::nullopt,
                                  "X,Y\n1,1\n2,0\n3,1e-15\n"),

                 SDVerifierParams("HugeFiniteGapRequiresDeletion", {0}, {1}, 0.0, 1e-10, 1, 0.5,
                                  false, {}, {}, std::nullopt, std::nullopt, "X,Y\n1,0\n2,1e308\n"),

                 SDVerifierParams("NonFiniteLhsRejected", {0}, {1}, 0.0, 1.0, 0, 1.0, false, {}, {},
                                  std::nullopt, std::nullopt, "X,Y\nnan,0\n2,1\n", true),

                 SDVerifierParams("NonFiniteRhsRejected", {0}, {1}, 0.0, 1.0, 0, 1.0, false, {}, {},
                                  std::nullopt, std::nullopt, "X,Y\n1,0\n2,inf\n", true)}),
        [](::testing::TestParamInfo<SDVerifierParams> const& info) { return info.param.name; });

}  // namespace tests
