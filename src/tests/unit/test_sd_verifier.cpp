#include <algorithm>
#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/sd/sd_verifier/sd_verifier.h"
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

    SDVerifierParams(std::string name, config::IndicesType lhs, config::IndicesType rhs, double g1,
                     double g2, long ops, double conf, bool check_violations = false,
                     std::vector<size_t> deletions = {},
                     std::vector<std::pair<size_t, size_t>> insertions = {},
                     std::optional<config::IndicesType> indices = std::nullopt,
                     std::optional<std::string> csv_filename = std::nullopt,
                     std::optional<std::string> csv_content = std::nullopt,
                     bool expect_exception = false)
        : name(std::move(name)),
          active_csv_filename(csv_filename.value_or("sd_test_gen_")),
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
        for (auto const& v : violations) {
            if (std::holds_alternative<algos::sd_verifier::SDInsertion>(v)) {
                auto const& ins = std::get<algos::sd_verifier::SDInsertion>(v);
                actual_insertions.emplace_back(ins.left_row_idx, ins.right_row_idx);
            } else {
                auto const& del = std::get<algos::sd_verifier::SDDeletion>(v);
                actual_deletions.emplace_back(del.row_idx);
            }
        }
        std::sort(actual_insertions.begin(), actual_insertions.end());
        std::sort(actual_deletions.begin(), actual_deletions.end());

        EXPECT_EQ(actual_deletions, p.expected_deletions);
        EXPECT_EQ(actual_insertions, p.expected_insertions);
    }
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
                                  std::nullopt, std::nullopt, "X,Y\n1,-15\n2,-5\n3,5\n4,15\n")}),
        [](::testing::TestParamInfo<SDVerifierParams> const& info) { return info.param.name; });

}  // namespace tests
