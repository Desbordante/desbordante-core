#include <gtest/gtest.h>

#include "algo_factory.h"
#include "algorithms/cfd/cfdfinder/cfdfinder.h"
#include "all_csv_configs.h"
#include "config/indices/option.h"
#include "config/names_and_descriptions.h"

namespace tests {

static void CheckCfdfinderSetsEquality(
        std::set<std::pair<std::string, std::vector<std::string>>> const& expected,
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

class CFDFinderAlgorithmTest : public ::testing::Test {
protected:
    static algos::StdParamsMap GetParamMap(algos::cfdfinder::Expansion expansion,
                                           algos::cfdfinder::Result result, unsigned int max_lhs,
                                           CSVConfig const& csv_config, double min_sup,
                                           double min_conf, bool is_null_equal_null) {
        using namespace config::names;
        return {{kCsvConfig, csv_config},
                {kMaximumLhs, max_lhs},
                {kCfdPruningStrategy, +algos::cfdfinder::Pruning::legacy},
                {kCfdMinimumSupport, min_sup},
                {kCfdMinimumConfidence, min_conf},
                {kEqualNulls, is_null_equal_null},
                {kCfdResultStrategy, result},
                {kCfdExpansionStrategy, +expansion}};
    }

    static algos::StdParamsMap GetParamMap(algos::cfdfinder::Expansion expansion,
                                           algos::cfdfinder::Result result, unsigned int max_lhs,
                                           CSVConfig const& csv_config, double min_conf,
                                           double min_support_gain, double max_level_support_drop,
                                           unsigned int pattern_threshold,
                                           bool is_null_equal_null) {
        using namespace config::names;
        return {{kCsvConfig, csv_config},
                {kMaximumLhs, max_lhs},
                {kCfdPruningStrategy, +algos::cfdfinder::Pruning::support_independent},
                {kCfdMinimumConfidence, min_conf},
                {kMinSupportGain, min_support_gain},
                {kMaxLevelSupportDrop, max_level_support_drop},
                {kPatternTreshold, pattern_threshold},
                {kEqualNulls, is_null_equal_null},
                {kCfdResultStrategy, result},
                {kCfdExpansionStrategy, +expansion}};
    }

    static algos::StdParamsMap GetParamMap(algos::cfdfinder::Expansion expansion,
                                           algos::cfdfinder::Result result, unsigned int max_lhs,
                                           CSVConfig const& csv_config, double min_conf,
                                           double min_support_gain, double max_level_support_drop,
                                           unsigned int pattern_threshold,
                                           config::IndicesType rhs_filter,
                                           bool is_null_equal_null) {
        using namespace config::names;
        return {{kCsvConfig, csv_config},
                {kMaximumLhs, max_lhs},
                {kCfdPruningStrategy, +algos::cfdfinder::Pruning::rhs_filter},
                {kCfdMinimumConfidence, min_conf},
                {kMinSupportGain, min_support_gain},
                {kMaxLevelSupportDrop, max_level_support_drop},
                {kPatternTreshold, pattern_threshold},
                {kEqualNulls, is_null_equal_null},
                {kRhsIndices, rhs_filter},
                {kCfdResultStrategy, result},
                {kCfdExpansionStrategy, +expansion}};
    }

    static algos::StdParamsMap GetParamMap(algos::cfdfinder::Expansion expansion,
                                           algos::cfdfinder::Result result, unsigned int max_lhs,
                                           CSVConfig const& csv_config, double max_g1,
                                           bool is_null_equal_null) {
        using namespace config::names;
        return {{kCsvConfig, csv_config},
                {kCfdResultStrategy, result},
                {kMaximumLhs, max_lhs},
                {kCfdPruningStrategy, +algos::cfdfinder::Pruning::partial_fd},
                {kCfdExpansionStrategy, +expansion},
                {kMaximumG1, max_g1},
                {kEqualNulls, is_null_equal_null},
                {kCfdExpansionStrategy, +expansion}};
    }

    template <typename... Args>
    static std::unique_ptr<algos::cfdfinder::CFDFinder> CreateAlgorithmInstance(
            algos::cfdfinder::Expansion expansion, algos::cfdfinder::Result result,
            unsigned int max_lhs, Args&&... args) {
        return algos::CreateAndLoadAlgorithm<algos::cfdfinder::CFDFinder>(
                GetParamMap(expansion, result, max_lhs, std::forward<Args>(args)...));
    }
};

TEST_F(CFDFinderAlgorithmTest, TennisConstantExpansionLegacyPruning) {
    algos::cfdfinder::PatternDebugController::SetDebugEnabled(true);
    unsigned int max_lhs = 4;
    double min_sup = 0.8;
    double min_conf = 1.0;
    bool is_null_eq_null = true;
    auto algorithm = CreateAlgorithmInstance(algos::cfdfinder::Expansion::constant,
                                             algos::cfdfinder::Result::direct, max_lhs, kTennis,
                                             min_sup, min_conf, is_null_eq_null);
    algorithm->Execute();

    std::set<std::pair<std::string, std::vector<std::string>>> expected = {
            {"[temp humidity windy play] -> outlook",
             {"_|high|_|_", "_|_|true|_", "mild|_|_|_", "hot|_|_|_"}},
            {"[outlook temp humidity play] -> windy", {"_|_|_|yes", "_|mild|_|_", "_|_|normal|_"}},
            {"[outlook temp play] -> windy", {"_|_|yes", "_|mild|_", "_|cool|_"}}};

    CheckCfdfinderSetsEquality(expected, algorithm->CfdList());
}

TEST_F(CFDFinderAlgorithmTest, TennisNegativeConstantExpansionLegacyPruning) {
    algos::cfdfinder::PatternDebugController::SetDebugEnabled(true);
    unsigned int max_lhs = 4;
    double min_sup = 0.7;
    double min_conf = 1.0;
    bool is_null_eq_null = true;
    auto algorithm = CreateAlgorithmInstance(algos::cfdfinder::Expansion::negative_constant,
                                             algos::cfdfinder::Result::tree, max_lhs, kTennis,
                                             min_sup, min_conf, is_null_eq_null);
    algorithm->Execute();
    std::set<std::pair<std::string, std::vector<std::string>>> expected = {
            {"[temp windy play] -> outlook", {"¬cool|false|_", "¬mild|¬false|_", "_|_|¬yes"}},
            {"[outlook temp windy] -> humidity",
             {"_|_|true", "¬overcast|¬mild|_", "¬rainy|mild|_"}},
            {"[outlook temp play] -> humidity", {"¬overcast|¬mild|_", "¬rainy|¬hot|_", "_|_|¬yes"}},
            {"[outlook humidity windy] -> temp", {"_|_|true", "¬sunny|high|_", "¬rainy|¬high|_"}},
            {"[outlook temp play] -> windy", {"_|¬hot|_"}},
            {"[outlook temp humidity] -> play", {"¬rainy|_|_", "_|mild|¬high"}},

    };

    CheckCfdfinderSetsEquality(expected, algorithm->CfdList());
}

TEST_F(CFDFinderAlgorithmTest, TennisRangeExpansionLegacyPruning) {
    algos::cfdfinder::PatternDebugController::SetDebugEnabled(true);
    unsigned int max_lhs = 4;
    double min_sup = 0.8;
    double min_conf = 1.0;
    bool is_null_eq_null = true;
    auto algorithm = CreateAlgorithmInstance(algos::cfdfinder::Expansion::range,
                                             algos::cfdfinder::Result::direct, max_lhs, kTennis,
                                             min_sup, min_conf, is_null_eq_null);
    algorithm->Execute();
    std::set<std::pair<std::string, std::vector<std::string>>> expected = {
            {"[temp humidity windy play] -> outlook",
             {"[cool - hot]|[high - normal]|[false - true]|[no - yes]",
              "[mild - mild]|[high - normal]|[true - true]|[no - yes]"}},
            {"[outlook temp humidity play] -> windy",
             {"[overcast - sunny]|[hot - mild]|[high - normal]|[no - yes]",
              "[overcast - rainy]|[cool - mild]|[high - normal]|[yes - yes]"}},
            {"[outlook temp play] -> windy",
             {"[overcast - sunny]|[hot - mild]|[no - yes]",
              "[overcast - rainy]|[cool - mild]|[yes - yes]"}}};

    CheckCfdfinderSetsEquality(expected, algorithm->CfdList());
}

TEST_F(CFDFinderAlgorithmTest, IrisConstantExpansionRhsFilterPruning) {
    algos::cfdfinder::PatternDebugController::SetDebugEnabled(true);
    config::IndicesType rhs = {0};
    unsigned int max_lhs = 4;
    double min_support_gain = 6;
    double max_support_drop = 15;
    unsigned int max_patterns = 2000;
    double min_conf = 1.0;
    bool is_null_eq_null = true;
    auto algorithm = CreateAlgorithmInstance(algos::cfdfinder::Expansion::constant,
                                             algos::cfdfinder::Result::lattice, max_lhs, kIris,
                                             min_conf, min_support_gain, max_support_drop,
                                             max_patterns, rhs, is_null_eq_null);
    algorithm->Execute();

    std::set<std::pair<std::string, std::vector<std::string>>> expected = {
            {"[1 2] -> 0", {"_|5.6", "3.3|_", "3.8|_"}},
            {"[2 3] -> 0", {"_|2.3", "5.1|_", "_|2.1"}},
    };

    CheckCfdfinderSetsEquality(expected, algorithm->CfdList());
}

TEST_F(CFDFinderAlgorithmTest, BridgesConstantExpansionRhsFilterPruning) {
    algos::cfdfinder::PatternDebugController::SetDebugEnabled(true);
    config::IndicesType rhs = {2};
    unsigned int max_lhs = 6;
    double min_support_gain = 35;
    double max_support_drop = 35;
    unsigned int max_patterns = 2000;
    double min_conf = 1.0;
    bool is_null_eq_null = true;
    auto algorithm = CreateAlgorithmInstance(algos::cfdfinder::Expansion::constant,
                                             algos::cfdfinder::Result::lattice, max_lhs, kBridges,
                                             min_conf, min_support_gain, max_support_drop,
                                             max_patterns, rhs, is_null_eq_null);
    algorithm->Execute();

    std::set<std::pair<std::string, std::vector<std::string>>> expected = {
            {"[1 4 5 6 9 10] -> 2", {"_|_|_|2|STEEL|_"}},
            {"[1 3 6 8 11 12] -> 2", {"M|_|_|_|_|_"}},
            {"[1 3 8 10 11] -> 2", {"M|_|_|_|_"}},
            {"[1 3 10 11 12] -> 2", {"M|_|_|_|_"}},
            {"[1 3 5] -> 2", {"A|_|_", "M|_|_"}},
            {"[3 5 6] -> 2", {"_|_|2"}}

    };

    CheckCfdfinderSetsEquality(expected, algorithm->CfdList());
}

TEST_F(CFDFinderAlgorithmTest, IrisConstantExpansionG1Pruning) {
    algos::cfdfinder::PatternDebugController::SetDebugEnabled(true);
    double max_g1 = 0.05;
    unsigned int max_lhs = 1;
    bool is_null_eq_null = true;
    auto algorithm = CreateAlgorithmInstance(algos::cfdfinder::Expansion::constant,
                                             algos::cfdfinder::Result::tree, max_lhs, kIris, max_g1,
                                             is_null_eq_null);
    algorithm->Execute();

    std::set<std::pair<std::string, std::vector<std::string>>> expected = {
            {"[0] -> 4", {"_"}}, {"[2] -> 0", {"_"}}, {"[0] -> 3", {"_"}}, {"[0] -> 1", {"_"}},
            {"[2] -> 1", {"_"}}, {"[0] -> 2", {"_"}}, {"[2] -> 3", {"_"}}, {"[3] -> 4", {"_"}},
            {"[1] -> 4", {"_"}}, {"[2] -> 4", {"_"}}

    };

    CheckCfdfinderSetsEquality(expected, algorithm->CfdList());
}

TEST_F(CFDFinderAlgorithmTest, NullEmptyTest) {
    algos::cfdfinder::PatternDebugController::SetDebugEnabled(true);
    unsigned int max_lhs = 100;
    bool is_null_eq_null = true;
    double min_sup = 0.3;
    double min_conf = 0.8;
    auto algorithm = CreateAlgorithmInstance(algos::cfdfinder::Expansion::constant,
                                             algos::cfdfinder::Result::direct, max_lhs, kNullEmpty,
                                             min_sup, min_conf, is_null_eq_null);
    algorithm->Execute();

    std::set<std::pair<std::string, std::vector<std::string>>> expected = {
            {"[Int1  IntAndEmpty  Int2] ->  NullAndInt", {"_|null|_"}},
            {"[Int1  NullAndInt  Int2] ->  IntAndEmpty", {"_|NULL|_"}},
            {"[Int1  NullAndInt] ->  IntAndEmpty", {"_|NULL"}},
            {"[ NullAndInt  Int2] ->  IntAndEmpty", {"NULL|_"}},
            {"[Int1  IntAndEmpty] ->  NullAndInt", {"_|null"}},
            {"[ IntAndEmpty  Int2] ->  NullAndInt", {"null|_"}},
            {"[ IntAndEmpty] ->  NullAndInt", {"null"}},
            {"[ NullAndInt] ->  IntAndEmpty", {"NULL"}}

    };

    CheckCfdfinderSetsEquality(expected, algorithm->CfdList());
}

}  // namespace tests
