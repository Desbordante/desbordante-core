#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/association_rules/ar_verifier/ar_verifier.h"
#include "all_csv_configs.h"
#include "config/names.h"

namespace tests {
namespace onam = config::names;

struct ARVerifyingParams {
    algos::StdParamsMap params;

    ARVerifyingParams(CSVConfig const& csv_config, std::list<std::string> left,
                      std::list<std::string> right, double min_sup, double min_conf,
                      algos::InputFormat format = algos::InputFormat::singular,
                      unsigned int tid_col = 0, unsigned int item_col = 1,
                      bool first_col_tid = true)
        : params({{onam::kCsvConfig, csv_config},
                  {onam::kInputFormat, +format},
                  {onam::kMinimumSupport, min_sup},
                  {onam::kMinimumConfidence, min_conf},
                  {onam::kARuleLeft, std::move(left)},
                  {onam::kARuleRight, std::move(right)},
                  {onam::kTIdColumnIndex, tid_col},
                  {onam::kItemColumnIndex, item_col},
                  {onam::kFirstColumnTId, first_col_tid}}) {}
};

class TestARVerifying : public ::testing::TestWithParam<ARVerifyingParams> {};

TEST_P(TestARVerifying, DefaultTest) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto verifier = algos::CreateAndLoadAlgorithm<algos::ar_verifier::ARVerifier>(mp);
    verifier->Execute();
    EXPECT_TRUE(verifier->ARHolds());
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    ARVerifierTestSuite, TestARVerifying,
    ::testing::Values(
        ARVerifyingParams(kRulesBook, {"Bread"}, {"Milk"}, 0.2, 0.1),
        ARVerifyingParams(kRulesPresentation, {"Bread"}, {"Diaper"}, 0.2, 0.1),
        ARVerifyingParams(kRulesPresentationExtended, {"Bread"}, {"Milk"}, 0.2, 0.1),
        ARVerifyingParams(kRulesBook, {"Eggs"}, {"Milk"}, 0.3, 0.1),
        ARVerifyingParams(kRulesBook, {"Yogurt"}, {"Eggs"}, 0.2, 0.1),
        ARVerifyingParams(kRulesBook, {"Yogurt"}, {"Milk"}, 0.2, 0.1)
    ));

INSTANTIATE_TEST_SUITE_P(
  ARVerifierRowsDataset, TestARVerifying,
  ::testing::Values(
      // test cases tabular
     ARVerifyingParams(kRulesKaggleRows, {"MILK"}, {"BREAD"}, 0.2, 0.1, algos::InputFormat::tabular, 0, 0, 1),
     ARVerifyingParams(kRulesKaggleRows, {"BREAD"}, {"MILK"}, 0.05, 0.2, algos::InputFormat::tabular, 0, 0, 1),
     ARVerifyingParams(kRulesKaggleRows, {"BREAD"}, {"TEA"}, 0.2, 0.1, algos::InputFormat::tabular, 0, 0, 1),
     ARVerifyingParams(kRulesKaggleRows, {"MAGGI"}, {"BISCUIT"}, 0.1, 0.05, algos::InputFormat::tabular, 0, 0, 1),
     ARVerifyingParams(kRulesKaggleRows, {"COFFEE"}, {"BISCUIT"}, 0.1, 0.25, algos::InputFormat::tabular, 0, 0, 1),
     ARVerifyingParams(kRulesKaggleRows, {"SUGER"}, {"CORNFLAKES"}, 0.05, 0.1, algos::InputFormat::tabular, 0, 0, 1)
  ));
// clang-format on
}  // namespace tests