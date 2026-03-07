#include <cstddef>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/dd/add_verifier/add_verifier.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
struct ADDVerifyingParams {
    algos::StdParamsMap params;
    double const error = 0.;
    std::size_t const num_error_pairs = 0;
    double const satisfaction_threshold = 0.;

    explicit ADDVerifyingParams(model::DDString const& add, std::size_t const num_error_pairs = 0,
                               double const error = 0., 
			       double const satisfaction_threshold = 0.,
			       CSVConfig const& csv_config = kTestDD)
        : params({{config::names::kCsvConfig, csv_config}, {config::names::kDDString, add}, {config::names::kSatisfactionThreshold, satisfaction_threshold}}),
          error(error),
          num_error_pairs(num_error_pairs),
	  satisfaction_threshold(satisfaction_threshold){}
};

class TestADDHoldsVerifying : public ::testing::TestWithParam<ADDVerifyingParams> {};

TEST_P(TestADDHoldsVerifying, ADDHoldsTest) {
    auto const& p = GetParam();
    auto const mp = algos::StdParamsMap(p.params);
    auto const verifier = algos::CreateAndLoadAlgorithm<algos::dd::ADDVerifier>(mp);
    verifier->Execute();
EXPECT_EQ(verifier->DDHolds(), (1. - p.error) >= p.satisfaction_threshold);
    EXPECT_DOUBLE_EQ(verifier->GetError(), p.error);
    EXPECT_EQ(verifier->GetNumErrorRhs(), p.num_error_pairs);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        ADDVerifierTestSuite, TestADDHoldsVerifying,
        ::testing::Values(
		ADDVerifyingParams({{{"Col0", 0, 2}},{{"Col1", 0, 11}}}, 1, 6./7., 1./8.), 
		ADDVerifyingParams({{{"Col0", 0, 2}},{{"Col1", 0, 11}}}, 1, 6./7., 1./6.), 
		ADDVerifyingParams({{{"Col0", 0, 2}},{{"Col1", 3, 10}}}, 2, 1., 1./10.), 
		ADDVerifyingParams({{{"Col0", 0, 2}},{{"Col1", 3, 10}}}, 2, 1., 1./4.), 
		ADDVerifyingParams({{{"Col0", 2, 3}},{{"Col1", 9, 10}}}, 4, 1., 1./15.), 
		ADDVerifyingParams({{{"Col0", 2, 3}},{{"Col1", 1, 2}}}, 5, 1., 2./7.), 
		ADDVerifyingParams({{{"Col0", 2, 3}},{{"Col1", 1, 2}}}, 5, 1., 0.), 
		ADDVerifyingParams({{{"Col0", 2, 3}},{{"Col1", 6, 7}}}, 4, 4./5., 1./6.), 
		ADDVerifyingParams({{{"Col0", 2, 3}},{{"Col1", 6, 7}}}, 4, 4./5., 1./8.)));

// clang-format on
}  // namespace tests
