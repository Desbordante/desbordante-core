#include <cstddef>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/dd/dd_verifier/dd_verifier.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
struct DDVerifyingParams {
    algos::StdParamsMap params;
    double const error = 0.;
    std::size_t const num_error_pairs = 0;

    explicit DDVerifyingParams(model::DDString const& dd, std::size_t const num_error_pairs = 0,
                               double const error = 0., CSVConfig const& csv_config = kTestDD)
        : params({{config::names::kCsvConfig, csv_config}, {config::names::kDDString, dd}}),
          error(error),
          num_error_pairs(num_error_pairs) {}
};

class TestDDHoldsVerifying : public ::testing::TestWithParam<DDVerifyingParams> {};

TEST_P(TestDDHoldsVerifying, DDHoldsTest) {
    auto const& p = GetParam();
    auto const mp = algos::StdParamsMap(p.params);
    auto const verifier = algos::CreateAndLoadAlgorithm<algos::dd::DDVerifier>(mp);
    verifier->Execute();
    EXPECT_EQ(verifier->DDHolds(), p.num_error_pairs == 0);
    EXPECT_DOUBLE_EQ(verifier->GetError(), p.error);
    EXPECT_EQ(verifier->GetNumErrorRhs(), p.num_error_pairs);
}

INSTANTIATE_TEST_SUITE_P(
        DDVerifierTestSuite, TestDDHoldsVerifying,
        ::testing::Values(DDVerifyingParams({{{"Col0", 0, 0}}, {{"Col1", 0, 0}}}, 0, 0.),
                          DDVerifyingParams({{{"Col0", 0, 2}}, {{"Col1", 0, 12}}}, 0, 0.),
                          DDVerifyingParams({{{"Col0", 1, 2}}, {{"Col1", 2, 12}}}, 0, 0.),
                          DDVerifyingParams({{{"Col0", 2, 3}}, {{"Col1", 6, 16}}}, 0, 0.),
                          DDVerifyingParams({{{"Col0", 2, 3}}, {{"Col1", 3, 16}}}, 0, 0.),
                          DDVerifyingParams({{{"Col1", 2, 16}}, {{"Col3", 111, 555}}}, 0, 0.),
                          DDVerifyingParams({{{"Col0", 0, 10000}}, {{"Col4", 0, 10000}}}, 0, 0.),
                          DDVerifyingParams({{{"Col0", 1, 1}, {"Col1", 4, 6}},
                                             {{"Col3", 222, 333}, {"Col4", 111, 555}}},
                                            0, 0.),
                          DDVerifyingParams({{{"Col0", 1, 1}, {"Col1", 4, 6}},
                                             {{"Col3", 222, 333}, {"Col4", 111, 550}}},
                                            1, 1. / 2.),
                          DDVerifyingParams({{{"Col0", 0, 2}}, {{"Col1", 0, 11}}}, 1, 1. / 7.),
                          DDVerifyingParams({{{"Col0", 0, 2}}, {{"Col1", 3, 10}}}, 2, 2. / 7.),
                          DDVerifyingParams({{{"Col0", 2, 3}}, {{"Col1", 9, 10}}}, 4, 4. / 5.),
                          DDVerifyingParams({{{"Col0", 2, 3}}, {{"Col1", 1, 2}}}, 5, 1.),
                          DDVerifyingParams({{{"Col1", 2, 16}}, {{"Col3", 112, 333}}}, 5,
                                            5. / 10.)));
}  // namespace tests
