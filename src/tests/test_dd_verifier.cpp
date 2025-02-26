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

        explicit DDVerifyingParams(const model::DDString &dd,
                                   size_t const num_error_pairs = 0,
                                   double const error = 0., CSVConfig const& csv_config = kTestDD)
        :   params({{config::names::kCsvConfig, csv_config}, {config::names::kDDString, dd}}),
            error(error),
            num_error_pairs(num_error_pairs) {}
};

class TestDDHoldsVerifying : public ::testing::TestWithParam<DDVerifyingParams> {};

TEST_P(TestDDHoldsVerifying, DDHoldsTest) {
    auto const& p = GetParam();
    const auto mp = algos::StdParamsMap(p.params);
    const auto verifier = algos::CreateAndLoadAlgorithm<algos::dd::DDVerifier>(mp);
    verifier->Execute();
    EXPECT_EQ(verifier->DDHolds(), p.num_error_pairs == 0);
    EXPECT_DOUBLE_EQ(verifier->GetError(), p.error);
    EXPECT_EQ(verifier->GetNumErrorRhs(), p.num_error_pairs);
}

//clang-format off
INSTANTIATE_TEST_SUITE_P(
        DDVerifierTestSuite, TestDDHoldsVerifying,
        ::testing::Values(
            DDVerifyingParams({{{"Col0", 0, 0}},{{"Col1", 0, 0}}}, 0, 0.),
            DDVerifyingParams({{{"Col0", 0, 2}},{{"Col1", 0, 12}}}, 0, 0.),
            DDVerifyingParams({{{"Col0", 1, 2}},{{"Col1", 2, 12}}}, 0, 0.),
            DDVerifyingParams({{{"Col0", 2, 3}},{{"Col1", 6, 16}}}, 0, 0.),
            DDVerifyingParams({{{"Col0", 2, 3}},{{"Col1", 3, 16}}}, 0, 0.),
            DDVerifyingParams({{{"Col1", 2,16}},{{"Col3", 111, 555}}}, 0, 0.),
            DDVerifyingParams({{{"Col0", 0, 10000}},{{"Col4",0,10000}}}, 0, 0.),
            DDVerifyingParams({{{"Col0", 1, 1},{"Col1", 4, 6}},{{"Col3", 222, 333},{"Col4",  111, 555}}}, 0, 0.)
            ));
//clang-format on

    class TestDDNotHoldsVerifying : public ::testing::TestWithParam<DDVerifyingParams> {};

    TEST_P(TestDDNotHoldsVerifying, DDNotHoldsTest) {
        auto const& p = GetParam();
        const auto mp = algos::StdParamsMap(p.params);
        const auto verifier = algos::CreateAndLoadAlgorithm<algos::dd::DDVerifier>(mp);
        verifier->Execute();
        EXPECT_EQ(verifier->DDHolds(), !p.num_error_pairs);
        EXPECT_DOUBLE_EQ(verifier->GetError(), p.error);
        EXPECT_EQ(verifier->GetNumErrorRhs(), p.num_error_pairs);
    }

    INSTANTIATE_TEST_SUITE_P(DDVerifierTestSuite, TestDDNotHoldsVerifying, ::testing::Values(
        DDVerifyingParams({{{"Col0", 0., 0.}},{{"Col2", 1.,1.}}}, 5, 1.),
        DDVerifyingParams({{{"Col0", 0, 2}},{{"Col1", 0, 11}}}, 1, 1./12.),
        DDVerifyingParams({{{"Col0", 0, 2}},{{"Col1", 3, 10}}}, 7, 7./12.),
        DDVerifyingParams({{{"Col0", 2, 3}},{{"Col1", 9, 10}}}, 4, 4./5.),
        DDVerifyingParams({{{"Col0", 2, 3}},{{"Col1", 1, 2}}}, 5, 1.),
        DDVerifyingParams({{{"Col1", 2,16}},{{"Col3", 112, 333}}}, 5, 5./10.),
        DDVerifyingParams({{{"Col0", 1, 1},{"Col1", 4, 6}},{{"Col3", 228, 333},{"Col4",  111, 550}}}, 1, 1./2.)
            ));
}  // namespace tests