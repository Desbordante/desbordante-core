#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/ar/ar_verifier/ar_verifier.h"
#include "core/config/names.h"
#include "core/model/transaction/input_format_type.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
namespace onam = config::names;

struct SingularDataset {
    CSVConfig csv_config;
    unsigned int tid_col;
    unsigned int item_col;
};

struct TabularDataset {
    CSVConfig csv_config;
    bool first_col_tid;
};

struct RuleConfig {
    std::vector<std::string> left;
    std::vector<std::string> right;
    double min_sup;
    double min_conf;
};

struct ARVerifierParams {
    algos::StdParamsMap params;

    ARVerifierParams(SingularDataset const& dataset, RuleConfig const& config)
        : params({{onam::kCsvConfig, dataset.csv_config},
                  {onam::kInputFormat, +model::InputFormatType::singular},
                  {onam::kArMinimumSupport, config.min_sup},
                  {onam::kArMinimumConfidence, config.min_conf},
                  {onam::kArLhsRule, config.left},
                  {onam::kArRhsRule, config.right},
                  {onam::kTIdColumnIndex, dataset.tid_col},
                  {onam::kItemColumnIndex, dataset.item_col}}) {}

    ARVerifierParams(TabularDataset const& dataset, RuleConfig const& config)
        : params({{onam::kCsvConfig, dataset.csv_config},
                  {onam::kInputFormat, +model::InputFormatType::tabular},
                  {onam::kArMinimumSupport, config.min_sup},
                  {onam::kArMinimumConfidence, config.min_conf},
                  {onam::kArLhsRule, config.left},
                  {onam::kArRhsRule, config.right},
                  {onam::kFirstColumnTId, dataset.first_col_tid}}) {}
};

SingularDataset const kRulesBookSingular{
        .csv_config = kRulesBook,
        .tid_col = 0,
        .item_col = 1,
};

SingularDataset const kRulesPresentationSingular{
        .csv_config = kRulesPresentation,
        .tid_col = 0,
        .item_col = 1,
};

SingularDataset const kRulesPresentationExtSingular{
        .csv_config = kRulesPresentationExtended,
        .tid_col = 0,
        .item_col = 1,
};

TabularDataset const kRulesKaggleRowsTabular{
        .csv_config = kRulesKaggleRows,
        .first_col_tid = true,
};

class TestARVerifier : public ::testing::TestWithParam<ARVerifierParams> {};

TEST_P(TestARVerifier, DefaultTest) {
    auto const& p = GetParam();
    auto verifier = algos::CreateAndLoadAlgorithm<algos::ar_verifier::ARVerifier>(p.params);
    verifier->Execute();
    EXPECT_TRUE(verifier->ARHolds());
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    ARVerifierTestSingular, TestARVerifier,
    ::testing::Values(
        ARVerifierParams(kRulesBookSingular,
                         {.left={"Bread"}, .right={"Milk"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifierParams(kRulesPresentationSingular,
                         {.left={"Bread"}, .right={"Diaper"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifierParams(kRulesPresentationExtSingular,
                         {.left={"Bread"}, .right={"Milk"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifierParams(kRulesBookSingular,
                         {.left={"Eggs"}, .right={"Milk"}, .min_sup=0.3, .min_conf=0.1}),
        ARVerifierParams(kRulesBookSingular,
                         {.left={"Yogurt"}, .right={"Eggs"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifierParams(kRulesBookSingular,
                         {.left={"Yogurt"}, .right={"Milk"}, .min_sup=0.2, .min_conf=0.1})
    ));

INSTANTIATE_TEST_SUITE_P(
    ARVerifierTestTabular, TestARVerifier,
    ::testing::Values(
        ARVerifierParams(kRulesKaggleRowsTabular,
                         {.left={"MILK"}, .right={"BREAD"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifierParams(kRulesKaggleRowsTabular,
                         {.left={"BREAD"}, .right={"MILK"}, .min_sup=0.05, .min_conf=0.2}),
        ARVerifierParams(kRulesKaggleRowsTabular,
                         {.left={"BREAD"}, .right={"TEA"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifierParams(kRulesKaggleRowsTabular,
                         {.left={"MAGGI"}, .right={"BISCUIT"}, .min_sup=0.1, .min_conf=0.05}),
        ARVerifierParams(kRulesKaggleRowsTabular,
                         {.left={"COFFEE"}, .right={"BISCUIT"}, .min_sup=0.1, .min_conf=0.25}),
        ARVerifierParams(kRulesKaggleRowsTabular,
                         {.left={"SUGER"}, .right={"CORNFLAKES"}, .min_sup=0.05, .min_conf=0.1})
    ));
// clang-format on

class TestARVerifierError : public ::testing::Test {
protected:
    static void CreateAndExecute(ARVerifierParams const& params) {
        auto verifier =
                algos::CreateAndLoadAlgorithm<algos::ar_verifier::ARVerifier>(params.params);
        verifier->Execute();
    }
};

TEST_F(TestARVerifierError, TestEmptyTable) {
    RuleConfig const config{
            .left = {"A"},
            .right = {"B"},
            .min_sup = 0.1,
            .min_conf = 0.1,
    };
    TabularDataset const empty_dataset{
            .csv_config = kTestEmpty,
            .first_col_tid = true,
    };
    ARVerifierParams params(empty_dataset, config);

    ASSERT_THROW(CreateAndExecute(params), std::runtime_error);
}

TEST_F(TestARVerifierError, TestInvalidSupport) {
    RuleConfig const config{
            .left = {"Bread"},
            .right = {"Milk"},
            .min_sup = 2.0,
            .min_conf = 0.1,
    };
    ARVerifierParams const params(kRulesBookSingular, config);

    ASSERT_THROW(CreateAndExecute(params), config::ConfigurationError);
}

TEST_F(TestARVerifierError, TestInvalidRule) {
    RuleConfig const config{
            .left = {"42"},
            .right = {"Milk"},
            .min_sup = 0.1,
            .min_conf = 0.1,
    };
    ARVerifierParams const params{kRulesBookSingular, config};

    ASSERT_THROW(CreateAndExecute(params), config::ConfigurationError);
}

TEST_F(TestARVerifierError, TestEmptyRule) {
    RuleConfig const config{
            .left = {},
            .right = {"Milk"},
            .min_sup = 0.1,
            .min_conf = 0.1,
    };
    ARVerifierParams const params{kRulesBookSingular, config};

    ASSERT_THROW(CreateAndExecute(params), config::ConfigurationError);
}
}  // namespace tests
