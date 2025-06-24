#include <iostream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "algo_factory.h"
#include "algorithms/fd/pattern_fd_verifier/pattern_fd_verifier.h"
#include "all_csv_configs.h"
#include "config/names_and_descriptions.h"

namespace tests {
namespace pf = algos::pattern_fd;

struct PatternFDVerifierParams {
    algos::StdParamsMap params;
    int expected_coverage;
    int expected_min_inclusion;
    double expected_max_deviation;
    bool expected_holds;

    PatternFDVerifierParams(config::IndicesType lhs, config::IndexType rhs,
                            pf::PatternsTable patterns, int min_cov, int min_inc, double max_dev,
                            CSVConfig const& csv_config = kLicenses, bool exp_holds = true,
                            int exp_cov = -1, int exp_inc = -1, double exp_dev = -1.0)
        : params({{config::names::kCsvConfig, csv_config},
                  {config::names::kEqualNulls, false},
                  {config::names::kLhsIndices, std::move(lhs)},
                  {"rhs_index", rhs},
                  {"patterns", std::move(patterns)},
                  {"min_pattern_fd_coverage", min_cov},
                  {"min_pattern_inclusion", min_inc},
                  {"max_rhs_deviation", max_dev}}),
          expected_coverage(exp_cov),
          expected_min_inclusion(exp_inc),
          expected_max_deviation(exp_dev),
          expected_holds(exp_holds) {};
};

class PatternFDVerifierTest : public ::testing::TestWithParam<PatternFDVerifierParams> {};

TEST_P(PatternFDVerifierTest, BasicVerification) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto verifier = algos::CreateAndLoadAlgorithm<pf::PatternFDVerifier>(mp);
    verifier->Execute();

    EXPECT_EQ(verifier->GetRealPatternFDCoverage(), p.expected_coverage);
    EXPECT_EQ(verifier->GetRealMinPatternInclusion(), p.expected_min_inclusion);
    EXPECT_DOUBLE_EQ(verifier->GetRealMaxRhsDeviation(), p.expected_max_deviation);

    EXPECT_EQ(verifier->PatternFDHolds(), p.expected_holds);
}

algos::pattern_fd::PatternsTable pattern_city_licensee_one = {
        {{2, std::make_shared<algos::pattern_fd::TokenPatternInfo>("GAITHERSBURG", 0)},
         {4, std::make_shared<algos::pattern_fd::RegexPatternInfo>("BBWLHR\\D{3}")}}};

algos::pattern_fd::PatternsTable pattern_city_licensee_two = {
        {{2, std::make_shared<algos::pattern_fd::TokenPatternInfo>("GAITHERSBURG", 0)},
         {4, std::make_shared<algos::pattern_fd::RegexPatternInfo>("BBWLHR\\D{3}")}},
        {{2, std::make_shared<algos::pattern_fd::TokenPatternInfo>("GERMANTOWN", 0)},
         {4, std::make_shared<algos::pattern_fd::RegexPatternInfo>("CBWLCC\\D{3}")}}};

algos::pattern_fd::PatternsTable pattern_city_licensee_no_lhs = {
        {{2, std::make_shared<algos::pattern_fd::TokenPatternInfo>("QWERTY", 0)},
         {4, std::make_shared<algos::pattern_fd::RegexPatternInfo>("BBWLHR\\D{3}")}}};

algos::pattern_fd::PatternsTable pattern_city_licensee_no_rhs = {
        {{2, std::make_shared<algos::pattern_fd::TokenPatternInfo>("GAITHERSBURG", 0)},
         {4, std::make_shared<algos::pattern_fd::RegexPatternInfo>("QWERTY")}}};

INSTANTIATE_TEST_SUITE_P(PatternFDVerifierAdditionalTests, PatternFDVerifierTest,
                         ::testing::Values(
                                 // 1. Positive tests
                                 PatternFDVerifierParams({2}, 4, pattern_city_licensee_one, 5, 5, 0,
                                                         kLicenses, true, 5, 5, 0),
                                 PatternFDVerifierParams({2}, 4, pattern_city_licensee_two, 8, 4,
                                                         0.5, kLicenses, true, 10, 5, 0.4),

                                 // 2. Negative tests
                                 PatternFDVerifierParams({2}, 4, pattern_city_licensee_one, 7, 5, 0,
                                                         kLicenses, false, 5, 5, 0),
                                 PatternFDVerifierParams({2}, 4, pattern_city_licensee_two, 11, 4,
                                                         0.5, kLicenses, false, 10, 5, 0.4),
                                 PatternFDVerifierParams({2}, 4, pattern_city_licensee_two, 8, 4,
                                                         0.1, kLicenses, false, 10, 5, 0.4),

                                 // 3. Edge-case tests
                                 PatternFDVerifierParams({2}, 4, pattern_city_licensee_no_lhs, 7, 5,
                                                         0, kLicenses, false, 0, 0, 0),
                                 PatternFDVerifierParams({2}, 4, pattern_city_licensee_no_rhs, 7, 5,
                                                         0, kLicenses, false, 5, 5, 1)));

}  // namespace tests