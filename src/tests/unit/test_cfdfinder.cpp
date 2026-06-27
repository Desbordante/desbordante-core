#include <boost/algorithm/string/join.hpp>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/cfd/cfdfinder/cfdfinder.h"
#include "core/config/indices/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/thread_number/type.h"
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
                    double min_conf, std::set<Excepted_CFD> excepted,
                    config::ThreadNumType thread_num = 1, bool is_null_equal_null = true,
                    config::IndicesType rhs_indices = {})
        : params({{config::names::kCsvConfig, csv_config},
                  {config::names::kMaximumLhs, max_lhs},
                  {config::names::kCfdPruningStrategy, algos::cfdfinder::Pruning::kLegacy},
                  {config::names::kCfdMinimumSupport, min_sup},
                  {config::names::kCfdMinimumConfidence, min_conf},
                  {config::names::kEqualNulls, is_null_equal_null},
                  {config::names::kCfdResultStrategy, result},
                  {config::names::kRhsIndices, rhs_indices},
                  {config::names::kCfdExpansionStrategy, expansion},
                  {config::names::kThreads, thread_num}}),
          excepted_cfds(std::move(excepted)) {}

    // support_independent strategy
    CFDFinderParams(CSVConfig csv_config, algos::cfdfinder::Expansion expansion,
                    algos::cfdfinder::Result result, unsigned int max_lhs, double min_conf,
                    double min_support_gain, double max_level_support_drop,
                    unsigned int pattern_threshold, std::set<Excepted_CFD> excepted,
                    config::IndicesType rhs_indices = {}, config::ThreadNumType thread_num = 1,
                    bool is_null_equal_null = true)
        : params({{config::names::kCsvConfig, csv_config},
                  {config::names::kMaximumLhs, max_lhs},
                  {config::names::kCfdPruningStrategy,
                   algos::cfdfinder::Pruning::kSupportIndependent},
                  {config::names::kCfdMinimumConfidence, min_conf},
                  {config::names::kMinSupportGain, min_support_gain},
                  {config::names::kMaxLevelSupportDrop, max_level_support_drop},
                  {config::names::kMaxPatterns, pattern_threshold},
                  {config::names::kEqualNulls, is_null_equal_null},
                  {config::names::kCfdResultStrategy, result},
                  {config::names::kRhsIndices, rhs_indices},
                  {config::names::kCfdExpansionStrategy, expansion},
                  {config::names::kThreads, thread_num}}),
          excepted_cfds(std::move(excepted)) {}

    // max_g1 strategy
    CFDFinderParams(CSVConfig csv_config, algos::cfdfinder::Expansion expansion,
                    algos::cfdfinder::Result result, unsigned int max_lhs, double max_g1,
                    std::set<Excepted_CFD> excepted, bool is_null_equal_null = true,
                    config::ThreadNumType thread_num = 1, config::IndicesType rhs_indices = {})
        : params({{config::names::kCsvConfig, csv_config},
                  {config::names::kCfdResultStrategy, result},
                  {config::names::kMaximumLhs, max_lhs},
                  {config::names::kCfdPruningStrategy, algos::cfdfinder::Pruning::kPartialFd},
                  {config::names::kCfdExpansionStrategy, expansion},
                  {config::names::kMaximumG1, max_g1},
                  {config::names::kRhsIndices, rhs_indices},
                  {config::names::kEqualNulls, is_null_equal_null},
                  {config::names::kThreads, thread_num}}),
          excepted_cfds(std::move(excepted)) {}
};

static void CheckEqualityExceptedCFDs(std::set<CFDFinderParams::Excepted_CFD> const& expected,
                                      std::list<algos::cfdfinder::CFD> const& actual) {
    ASSERT_EQ(actual.size(), expected.size()) << "count of cfds does not match: expected "
                                              << expected.size() << ", got " << actual.size();

    for (auto const& cfd : actual) {
        auto embedded_fd = cfd.GetEmbeddedFD().ToLongString();

        std::vector<std::string> patterns;
        for (auto const& pattern : cfd.GetTableau()) {
            patterns.push_back(boost::algorithm::join(pattern, "|"));
        }
        std::pair<std::string, std::vector<std::string>> expected_cfd = {std::move(embedded_fd),
                                                                         std::move(patterns)};
        if (expected.find(expected_cfd) == expected.end()) {
            FAIL() << "generated cfd not found in expected";
        }
    }
    SUCCEED();
}

class CFDFinderAlgorithmTest : public ::testing::TestWithParam<CFDFinderParams> {};

TEST_P(CFDFinderAlgorithmTest, Test) {
    std::ofstream file("/home/oddin60f/codes/metanome-algorithms/old_test_c++.txt",
                       std::ios::trunc);
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto algo = algos::CreateAndLoadAlgorithm<algos::cfdfinder::CFDFinder>(mp);
    algo->Execute();
    for (auto const& cfd : algo->CfdList()) {
        file << cfd.ToString();
    }
    CheckEqualityExceptedCFDs(p.excepted_cfds, algo->CfdList());
}

INSTANTIATE_TEST_SUITE_P(
        CFDFinderAdditionalTests, CFDFinderAlgorithmTest,
        ::testing::Values(
                CFDFinderParams({kTennis,
                                 algos::cfdfinder::Expansion::kConstant,
                                 algos::cfdfinder::Result::kDirect,
                                 4,    // max_lhs
                                 0.8,  // min_sup
                                 1.0,  // min_conf
                                 {
                                         {"[temp humidity windy play] -> outlook",
                                          {"_|high|_|_", "_|_|true|_", "hot|_|_|_", "mild|_|_|_"}},
                                         {"[outlook temp humidity play] -> windy",
                                          {"_|_|_|yes", "_|mild|_|_", "_|_|normal|_"}},
                                         {"[outlook temp play] -> windy",
                                          {"_|_|yes", "_|mild|_", "_|cool|_"}},
                                 }}),
                CFDFinderParams({kTennis,
                                 algos::cfdfinder::Expansion::kNegativeConstant,
                                 algos::cfdfinder::Result::kDirect,
                                 4,    // max_lhs
                                 0.8,  // min_sup
                                 1.0,  // min_conf
                                 {
                                         {"[temp humidity windy play] -> outlook",
                                          {"¬cool|_|_|_", "_|_|true|_"}},
                                         {"[outlook temp humidity play] -> windy",
                                          {"_|¬hot|_|_", "_|_|_|yes"}},
                                         {"[outlook temp play] -> windy", {"_|¬hot|_", "_|_|yes"}},
                                 }}),
                CFDFinderParams(
                        {kTennis,
                         algos::cfdfinder::Expansion::kRange,
                         algos::cfdfinder::Result::kDirect,
                         4,    // max_lhs
                         0.8,  // min_sup
                         1.0,  // min_conf
                         {
                                 {"[temp humidity windy play] -> outlook",
                                  {"[cool - hot]|[high - normal]|[false - true]|[no - yes]",
                                   "[cool - mild]|[high - normal]|[true - true]|[no - yes]"}},
                                 {"[outlook temp humidity play] -> windy",
                                  {"[overcast - sunny]|[hot - mild]|[high - normal]|[no - yes]",
                                   "[overcast - rainy]|[cool - cool]|[high - normal]|[yes - yes]"}},
                                 {"[outlook temp play] -> windy",
                                  {"[overcast - sunny]|[hot - mild]|[no - yes]",
                                   "[overcast - rainy]|[cool - cool]|[yes - yes]"}},
                         }}),
                CFDFinderParams({kIris,
                                 algos::cfdfinder::Expansion::kConstant,
                                 algos::cfdfinder::Result::kLattice,
                                 4,    // max_lhs
                                 1.0,  // min_conf
                                 6,    // min_support_gain
                                 15,   // max_support_drop
                                 4,    // max_patterns
                                 {
                                         {"[1 2] -> 0", {"_|5.6", "3.8|_", "3.3|_"}},
                                         {"[2 3] -> 0", {"_|2.3", "5.1|_", "_|2.1"}},
                                 },
                                 {0}}),
                CFDFinderParams({kBridges,
                                 algos::cfdfinder::Expansion::kConstant,
                                 algos::cfdfinder::Result::kLattice,
                                 6,    // max_lhs
                                 1.0,  // min_conf
                                 35,   // min_support_gain
                                 15,   // max_support_drop
                                 200,  // max_patterns
                                 {
                                         {"[1 4 5 6 9 10] -> 2", {"_|_|_|2|STEEL|_"}},
                                         {"[1 3 6 8 11 12] -> 2", {"M|_|_|_|_|_"}},
                                         {"[1 3 8 10 11] -> 2", {"M|_|_|_|_"}},
                                         {"[1 3 10 11 12] -> 2", {"M|_|_|_|_"}},
                                         {"[1 3 5] -> 2", {"A|_|_", "M|_|_"}},
                                         {"[3 5 6] -> 2", {"_|_|2"}},

                                 },
                                 {2},  // rhs_indices
                                 2

                }),
                CFDFinderParams({kIris,
                                 algos::cfdfinder::Expansion::kConstant,
                                 algos::cfdfinder::Result::kTree,
                                 1,     // max_lhs
                                 0.05,  // max_g1
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
                                 algos::cfdfinder::Expansion::kConstant,
                                 algos::cfdfinder::Result::kDirect,
                                 52,
                                 0.3,
                                 0.8,
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
