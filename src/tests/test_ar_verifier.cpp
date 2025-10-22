#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/association_rules/ar_verifier/ar_verifier.h"
#include "all_csv_configs.h"
#include "config/names.h"

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
    std::list<std::string> left;
    std::list<std::string> right;
    double min_sup;
    double min_conf;
};

struct ARVerifyingParams {
    algos::StdParamsMap params;

    ARVerifyingParams(SingularDataset const& dataset, RuleConfig config)
        : params({{onam::kCsvConfig, dataset.csv_config},
                  {onam::kInputFormat, +algos::InputFormat::singular},
                  {onam::kMinimumSupport, config.min_sup},
                  {onam::kMinimumConfidence, config.min_conf},
                  {onam::kARuleLeft, std::move(config.left)},
                  {onam::kARuleRight, std::move(config.right)},
                  {onam::kTIdColumnIndex, dataset.tid_col},
                  {onam::kItemColumnIndex, dataset.item_col}}) {}

    ARVerifyingParams(TabularDataset const& dataset, RuleConfig config)
        : params({{onam::kCsvConfig, dataset.csv_config},
                  {onam::kInputFormat, +algos::InputFormat::tabular},
                  {onam::kMinimumSupport, config.min_sup},
                  {onam::kMinimumConfidence, config.min_conf},
                  {onam::kARuleLeft, std::move(config.left)},
                  {onam::kARuleRight, std::move(config.right)},
                  {onam::kFirstColumnTId, dataset.first_col_tid}}) {}
};

SingularDataset const kRulesBookSingular{kRulesBook, 0, 1};
SingularDataset const kRulesPresentationSingular{kRulesPresentation, 0, 1};
SingularDataset const kRulesPresentationExtSingular{kRulesPresentationExtended, 0, 1};
TabularDataset const kRulesKaggleRowsTabular{kRulesKaggleRows, true};

class TestARVerifying : public ::testing::TestWithParam<ARVerifyingParams> {};

TEST_P(TestARVerifying, DefaultTest) {
    auto const& p = GetParam();
    auto verifier = algos::CreateAndLoadAlgorithm<algos::ar_verifier::ARVerifier>(p.params);
    verifier->Execute();
    EXPECT_TRUE(verifier->ARHolds());
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    ARVerifyingTestSingular, TestARVerifying,
    ::testing::Values(
        ARVerifyingParams(kRulesBookSingular,
                         {.left={"Bread"}, .right={"Milk"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifyingParams(kRulesPresentationSingular,
                         {.left={"Bread"}, .right={"Diaper"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifyingParams(kRulesPresentationExtSingular,
                         {.left={"Bread"}, .right={"Milk"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifyingParams(kRulesBookSingular,
                         {.left={"Eggs"}, .right={"Milk"}, .min_sup=0.3, .min_conf=0.1}),
        ARVerifyingParams(kRulesBookSingular,
                         {.left={"Yogurt"}, .right={"Eggs"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifyingParams(kRulesBookSingular,
                         {.left={"Yogurt"}, .right={"Milk"}, .min_sup=0.2, .min_conf=0.1})
    ));

INSTANTIATE_TEST_SUITE_P(
    ARVerifyingTestTabular, TestARVerifying,
    ::testing::Values(
        ARVerifyingParams(kRulesKaggleRowsTabular,
                         {.left={"MILK"}, .right={"BREAD"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifyingParams(kRulesKaggleRowsTabular,
                         {.left={"BREAD"}, .right={"MILK"}, .min_sup=0.05, .min_conf=0.2}),
        ARVerifyingParams(kRulesKaggleRowsTabular,
                         {.left={"BREAD"}, .right={"TEA"}, .min_sup=0.2, .min_conf=0.1}),
        ARVerifyingParams(kRulesKaggleRowsTabular,
                         {.left={"MAGGI"}, .right={"BISCUIT"}, .min_sup=0.1, .min_conf=0.05}),
        ARVerifyingParams(kRulesKaggleRowsTabular,
                         {.left={"COFFEE"}, .right={"BISCUIT"}, .min_sup=0.1, .min_conf=0.25}),
        ARVerifyingParams(kRulesKaggleRowsTabular,
                         {.left={"SUGER"}, .right={"CORNFLAKES"}, .min_sup=0.05, .min_conf=0.1})
    ));
// clang-format on

class TestARVerifierError : public ::testing::Test {
protected:
    static void CreateAndExecute(ARVerifyingParams const& params) {
        auto verifier =
                algos::CreateAndLoadAlgorithm<algos::ar_verifier::ARVerifier>(params.params);
        verifier->Execute();
    }
};

TEST_F(TestARVerifierError, TestEmptyTable) {
    TabularDataset empty_dataset{kTestEmpty, true};
    RuleConfig config{{"A"}, {"B"}, 0.1, 0.1};
    ARVerifyingParams params(empty_dataset, config);

    ASSERT_THROW(CreateAndExecute(params), std::runtime_error);
}

TEST_F(TestARVerifierError, TestInvalidSupport) {
    SingularDataset dataset{kRulesBook, 0, 1};
    RuleConfig config{{"Bread"}, {"Milk"}, 2.0, 0.1};
    ARVerifyingParams params(dataset, config);

    ASSERT_THROW(CreateAndExecute(params), config::ConfigurationError);
}

TEST_F(TestARVerifierError, TestInvalidRule) {
    SingularDataset dataset{kRulesBook, 0, 1};
    RuleConfig config{{"42"}, {"Milk"}, 0.1, 0.1};
    ARVerifyingParams params{dataset, config};

    ASSERT_THROW(CreateAndExecute(params), config::ConfigurationError);
}

TEST_F(TestARVerifierError, TestEmptyRule) {
    SingularDataset dataset{kRulesBook, 0, 1};
    RuleConfig config{{}, {"Milk"}, 0.1, 0.1};
    ARVerifyingParams params{dataset, config};

    ASSERT_THROW(CreateAndExecute(params), config::ConfigurationError);
}
}  // namespace tests
