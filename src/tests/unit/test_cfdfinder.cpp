#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string/join.hpp>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/cfd/cfdfinder/cfdfinder.h"
#include "core/config/indices/option.h"
#include "core/config/names_and_descriptions.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
struct CFDFinderParams {
    using FD = std::string;
    using Tableau = std::vector<std::string>;
    using Excepted_CFD = std::pair<FD, Tableau>;
    algos::StdParamsMap params;
    std::set<Excepted_CFD> excepted_cfds;

    // legacy strategy
    CFDFinderParams(CSVConfig csv_config, algos::cfdfinder::Expansion expansion,
                    algos::cfdfinder::Result result, unsigned int max_lhs, double min_sup,
                    double min_conf, bool is_null_equal_null, std::set<Excepted_CFD> excepted)
        : params({{config::names::kCsvConfig, csv_config},
                  {config::names::kMaximumLhs, max_lhs},
                  {config::names::kCfdPruningStrategy, +algos::cfdfinder::Pruning::legacy},
                  {config::names::kCfdMinimumSupport, min_sup},
                  {config::names::kCfdMinimumConfidence, min_conf},
                  {config::names::kEqualNulls, is_null_equal_null},
                  {config::names::kCfdResultStrategy, +result},
                  {config::names::kCfdExpansionStrategy, +expansion}}),
          excepted_cfds(std::move(excepted)) {}

    // support_independent strategy
    CFDFinderParams(CSVConfig csv_config, algos::cfdfinder::Expansion expansion,
                    algos::cfdfinder::Result result, unsigned int max_lhs, double min_conf,
                    double min_support_gain, double max_level_support_drop,
                    unsigned int pattern_threshold, bool is_null_equal_null,
                    std::set<Excepted_CFD> excepted)
        : params({{config::names::kCsvConfig, csv_config},
                  {config::names::kMaximumLhs, max_lhs},
                  {config::names::kCfdPruningStrategy,
                   +algos::cfdfinder::Pruning::support_independent},
                  {config::names::kCfdMinimumConfidence, min_conf},
                  {config::names::kMinSupportGain, min_support_gain},
                  {config::names::kMaxLevelSupportDrop, max_level_support_drop},
                  {config::names::kMaxPatterns, pattern_threshold},
                  {config::names::kEqualNulls, is_null_equal_null},
                  {config::names::kCfdResultStrategy, +result},
                  {config::names::kCfdExpansionStrategy, +expansion}}),
          excepted_cfds(std::move(excepted)) {}

    // rhs_filter strategy
    CFDFinderParams(CSVConfig csv_config, algos::cfdfinder::Expansion expansion,
                    algos::cfdfinder::Result result, unsigned int max_lhs, double min_conf,
                    double min_support_gain, double max_level_support_drop,
                    unsigned int pattern_threshold, config::IndicesType rhs_indeces,
                    bool is_null_equal_null, std::set<Excepted_CFD> excepted)
        : params({{config::names::kCsvConfig, csv_config},
                  {config::names::kMaximumLhs, max_lhs},
                  {config::names::kCfdPruningStrategy, +algos::cfdfinder::Pruning::rhs_filter},
                  {config::names::kCfdMinimumConfidence, min_conf},
                  {config::names::kMinSupportGain, min_support_gain},
                  {config::names::kMaxLevelSupportDrop, max_level_support_drop},
                  {config::names::kMaxPatterns, pattern_threshold},
                  {config::names::kEqualNulls, is_null_equal_null},
                  {config::names::kRhsIndices, rhs_indeces},
                  {config::names::kCfdResultStrategy, +result},
                  {config::names::kCfdExpansionStrategy, +expansion}}),
          excepted_cfds(std::move(excepted)) {}

    // max_g1 strategy
    CFDFinderParams(CSVConfig csv_config, algos::cfdfinder::Expansion expansion,
                    algos::cfdfinder::Result result, unsigned int max_lhs, double max_g1,
                    bool is_null_equal_null, std::set<Excepted_CFD> excepted)
        : params({{config::names::kCsvConfig, csv_config},
                  {config::names::kCfdResultStrategy, +result},
                  {config::names::kMaximumLhs, max_lhs},
                  {config::names::kCfdPruningStrategy, +algos::cfdfinder::Pruning::partial_fd},
                  {config::names::kCfdExpansionStrategy, +expansion},
                  {config::names::kMaximumG1, max_g1},
                  {config::names::kEqualNulls, is_null_equal_null},
                  {config::names::kCfdExpansionStrategy, +expansion}}),
          excepted_cfds(std::move(excepted)) {}
};

static void CheckEqualityExceptedCFDs(std::set<CFDFinderParams::Excepted_CFD> const& expected,
                                      std::list<algos::cfdfinder::CFD> const& actual) {
    ASSERT_EQ(actual.size(), expected.size()) << "count of cfds does not match: expected "
                                              << expected.size() << ", got " << actual.size();

    for (auto const& cfd : actual) {
        auto embeded_fd = cfd.GetEmbeddedFD().ToLongString();

        std::vector<std::string> patterns;
        for (auto const& pattern : cfd.GetTableau()) {
            patterns.push_back(boost::algorithm::join(pattern, "|"));
        }
        std::pair<std::string, std::vector<std::string>> expected_cfd = {std::move(embeded_fd),
                                                                         std::move(patterns)};
        if (expected.find(expected_cfd) == expected.end()) {
            FAIL() << "generated cfd not found in expected";
        }
    }
    SUCCEED();
}

class CFDFinderAlgorithmTest : public ::testing::TestWithParam<CFDFinderParams> {};

TEST_P(CFDFinderAlgorithmTest, Test) {
    algos::cfdfinder::PatternDebugController::SetDebugEnabled(true);

    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto algo = algos::CreateAndLoadAlgorithm<algos::cfdfinder::CFDFinder>(mp);
    algo->Execute();

    CheckEqualityExceptedCFDs(p.excepted_cfds, algo->CfdList());
}

INSTANTIATE_TEST_SUITE_P(
        CFDFinderAdditionalTests, CFDFinderAlgorithmTest,
        ::testing::Values(
                CFDFinderParams({kTennis,
                                 algos::cfdfinder::Expansion::constant,
                                 algos::cfdfinder::Result::direct,
                                 4,     // max_lhs
                                 0.8,   // min_sup
                                 1.0,   // min_conf
                                 true,  // is_null_equal_null
                                 {
                                         {"[temp humidity windy play] -> outlook",
                                          {"_|high|_|_", "_|_|true|_", "mild|_|_|_", "hot|_|_|_"}},
                                         {"[outlook temp humidity play] -> windy",
                                          {"_|_|_|yes", "_|mild|_|_", "_|_|normal|_"}},
                                         {"[outlook temp play] -> windy",
                                          {"_|_|yes", "_|mild|_", "_|cool|_"}},
                                 }}),
                CFDFinderParams({kTennis,
                                 algos::cfdfinder::Expansion::negative_constant,
                                 algos::cfdfinder::Result::tree,
                                 4,
                                 0.7,
                                 1.0,
                                 true,
                                 {
                                         {"[temp windy play] -> outlook",
                                          {"¬cool|false|_", "¬mild|¬false|_", "_|_|¬yes"}},
                                         {"[outlook temp windy] -> humidity",
                                          {"_|_|true", "¬overcast|¬mild|_", "¬rainy|mild|_"}},
                                         {"[outlook temp play] -> humidity",
                                          {"¬overcast|¬mild|_", "¬rainy|¬hot|_", "_|_|¬yes"}},
                                         {"[outlook humidity windy] -> temp",
                                          {"_|_|true", "¬sunny|high|_", "¬rainy|¬high|_"}},
                                         {"[outlook temp play] -> windy", {"_|¬hot|_"}},
                                         {"[outlook temp humidity] -> play",
                                          {"¬rainy|_|_", "_|mild|¬high"}},
                                 }}),
                CFDFinderParams({kTennis,
                                 algos::cfdfinder::Expansion::range,
                                 algos::cfdfinder::Result::direct,
                                 4,
                                 0.8,
                                 1.0,
                                 true,
                                 {
                                         {"[temp humidity windy play] -> outlook",
                                          {"[cool - hot]|_|_|_", "mild|_|true|_"}},
                                         {"[outlook temp humidity play] -> windy",
                                          {"_|[hot - mild]|_|_", "[overcast - rainy]|_|_|yes"}},
                                         {"[outlook temp play] -> windy",
                                          {"_|[hot - mild]|_", "[overcast - rainy]|_|yes"}},
                                 }}),
                CFDFinderParams({kIris,
                                 algos::cfdfinder::Expansion::constant,
                                 algos::cfdfinder::Result::lattice,
                                 4,     // max_lhs
                                 1.0,   // min_conf
                                 6,     // min_support_gain
                                 15,    // max_support_drop
                                 2000,  // max_patterns
                                 {0},   // rhs_indeces
                                 true,  // is_null_equal_null
                                 {
                                         {"[1 2] -> 0", {"_|5.6", "3.3|_", "3.8|_"}},
                                         {"[2 3] -> 0", {"_|2.3", "5.1|_", "_|2.1"}},
                                 }}),
                CFDFinderParams({kBridges,
                                 algos::cfdfinder::Expansion::constant,
                                 algos::cfdfinder::Result::lattice,
                                 6,
                                 1.0,
                                 35,
                                 35,
                                 2000,
                                 {2},
                                 true,
                                 {
                                         {"[1 4 5 6 9 10] -> 2", {"_|_|_|2|STEEL|_"}},
                                         {"[1 3 6 8 11 12] -> 2", {"M|_|_|_|_|_"}},
                                         {"[1 3 8 10 11] -> 2", {"M|_|_|_|_"}},
                                         {"[1 3 10 11 12] -> 2", {"M|_|_|_|_"}},
                                         {"[1 3 5] -> 2", {"A|_|_", "M|_|_"}},
                                         {"[3 5 6] -> 2", {"_|_|2"}},

                                 }}),
                CFDFinderParams({kIris,
                                 algos::cfdfinder::Expansion::constant,
                                 algos::cfdfinder::Result::tree,
                                 1,     // max_lhs
                                 0.05,  // max_g1
                                 true,
                                 {
                                         {"[0] -> 4", {"_"}},
                                         {"[2] -> 0", {"_"}},
                                         {"[0] -> 3", {"_"}},
                                         {"[0] -> 1", {"_"}},
                                         {"[2] -> 1", {"_"}},
                                         {"[0] -> 2", {"_"}},
                                         {"[2] -> 3", {"_"}},
                                         {"[3] -> 4", {"_"}},
                                         {"[1] -> 4", {"_"}},
                                         {"[2] -> 4", {"_"}},

                                 }}),
                CFDFinderParams({kNullEmpty,
                                 algos::cfdfinder::Expansion::constant,
                                 algos::cfdfinder::Result::direct,
                                 52,
                                 0.3,
                                 0.8,
                                 true,
                                 {
                                         {"[Int1  IntAndEmpty  Int2] ->  NullAndInt", {"_|null|_"}},
                                         {"[Int1  NullAndInt  Int2] ->  IntAndEmpty", {"_|NULL|_"}},
                                         {"[Int1  NullAndInt] ->  IntAndEmpty", {"_|NULL"}},
                                         {"[ NullAndInt  Int2] ->  IntAndEmpty", {"NULL|_"}},
                                         {"[Int1  IntAndEmpty] ->  NullAndInt", {"_|null"}},
                                         {"[ IntAndEmpty  Int2] ->  NullAndInt", {"null|_"}},
                                         {"[ IntAndEmpty] ->  NullAndInt", {"null"}},
                                         {"[ NullAndInt] ->  IntAndEmpty", {"NULL"}},

                                 }})));

}  // namespace tests
