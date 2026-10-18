#include <cstddef>
#include <utility>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/cdd/cdd.h"
#include "core/algorithms/cdd/cdd_verifier/cdd_verifier.h"
#include "core/algorithms/dd/dd_verifier/Metric.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
using namespace model;

struct CDDVerifyingParams {
    algos::StdParamsMap params;
    double const error = 0.;
    std::size_t const num_error_pairs = 0;
    std::size_t const num_cond_violations = 0;

    CDDVerifyingParams(CDD const& cdd, std::size_t const num_error_pairs = 0,
                       double const error = 0., std::size_t const num_cond_violations = 0,
                       std::unordered_map<std::string, std::shared_ptr<Metric>> metrics = {},
                       CSVConfig const& csv_config = kTestDD)
        : params({{config::names::kCsvConfig, csv_config},
                  {config::names::kCdd, cdd},
                  {config::names::kDDudm, metrics}}),
          error(error),
          num_error_pairs(num_error_pairs),
          num_cond_violations(num_cond_violations) {}
};

class TestCDDHoldsVerifying : public ::testing::TestWithParam<CDDVerifyingParams> {};

TEST_P(TestCDDHoldsVerifying, CDDHoldsTest) {
    auto const& p = GetParam();
    auto const mp = algos::StdParamsMap(p.params);
    auto const verifier = algos::CreateAndLoadAlgorithm<algos::cdd::CDDVerifier>(mp);
    verifier->Execute();
    EXPECT_EQ(verifier->DDHolds(), p.num_error_pairs == 0 && p.num_cond_violations == 0);
    EXPECT_DOUBLE_EQ(verifier->GetError(), p.error);
    EXPECT_EQ(verifier->GetNumErrorRhs(), p.num_error_pairs);
}

static int64_t val1 = 1, val3 = 3, val4 = 4, val5 = 5;
static int64_t val10 = 10, val470 = 470;

INSTANTIATE_TEST_SUITE_P(
        DDVerifierTestSuite, TestCDDHoldsVerifying,
        ::testing::Values(
                CDDVerifyingParams({{{{"Col0", 0, 0}}, {{"Col1", 0, 0}}}, {}, {}}, 0, 0., 0),
                CDDVerifyingParams({{{{"Col0", 0, 2}}, {{"Col1", 4, 8}}},
                                    {Condition("Col0", val4, ConditionOp::NEQ)},
                                    {}},
                                   0, 0., 0),
                CDDVerifyingParams({{{{"Col0", 0, 1}}, {{"Col2", 0, 50}}},
                                    {Condition("Col0", val4, ConditionOp::LT),
                                     Condition("Col1", val10, ConditionOp::LE)},
                                    {}},
                                   0, 0., 0),

                CDDVerifyingParams({{{{"Col0", 0, 2}}, {{"Col1", 6, 8}}},
                                    {Condition("Col0",
                                               std::vector<model::ConditionValue>{val1, val3, val5},
                                               ConditionOp::IN_SET)},
                                    {}},
                                   0, 0., 0),
                CDDVerifyingParams({{{{"Col0", 0, 1}}, {{"Col1", 2, 8}}},
                                    {Condition("Col0",
                                               std::make_pair(model::ConditionValue(val3),
                                                              model::ConditionValue(val5)),
                                               ConditionOp::IN_INTERVAL)},
                                    {}},
                                   0, 0., 0),
                CDDVerifyingParams({{{{"Col0", 0, 2}}, {{"Col1", 4, 8}}},
                                    {Condition("Col0", val4, ConditionOp::LT)},
                                    {Condition("Col2", val470, ConditionOp::GT),
                                     Condition("Col2", val470, ConditionOp::GE)}},
                                   0, 0., 1)));

// clang-format on
}  // namespace tests
