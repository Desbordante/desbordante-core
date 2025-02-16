#include <boost/parameter/aux_/void.hpp>
#include <gtest/gtest.h>

#include "algo_factory.h"
#include "all_csv_configs.h"
#include "dd/dd_verifier/dd_verifier.h"

namespace tests {
    struct DDVerifyingParams {
    algos::StdParamsMap params;
    double const error = 0.;
    size_t const num_error_pairs = 0;

    DDVerifyingParams(model::DDString dd,
                      size_t const num_error_pairs = 0,
                      long double const error = 0., CSVConfig const& csv_config = kTestDD)
        : params({{config::names::kCsvConfig, csv_config},
                    {config::names::kDDString, dd}}),
          error(error),
          num_error_pairs(num_error_pairs) {}
};

class TestDDVerifying : public ::testing::TestWithParam<DDVerifyingParams> {};

TEST_P(TestDDVerifying, BaseTest) {
    auto const& p = GetParam();
    const auto mp = algos::StdParamsMap(p.params);
    const auto verifier = algos::CreateAndLoadAlgorithm<algos::dd::DDVerifier>(mp);
    verifier->Execute();
    EXPECT_EQ(verifier->DDHolds(), p.num_error_pairs == 0);
    EXPECT_DOUBLE_EQ(verifier->GetError(), p.error);
    EXPECT_EQ(verifier->GetNumErrorPairs(), p.num_error_pairs);
}

//clang-format off
INSTANTIATE_TEST_SUITE_P(
        DDVerifierTestSuite, TestDDVerifying,
        ::testing::Values(

            ));
//clang-format on

}  // namespace tests