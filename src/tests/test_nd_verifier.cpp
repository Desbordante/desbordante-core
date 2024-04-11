#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/nd/nd.h"
#include "algorithms/nd/nd_verifier/nd_verifier.h"
#include "all_csv_configs.h"
#include "config/names.h"

namespace tests {
namespace onam = config::names;

struct NDVerifyingParams {
    algos::StdParamsMap params;

    NDVerifyingParams(config::IndicesType lhs_indices, config::IndicesType rhs_indices,
                      model::WeightType weight = -1, CSVConfig const& csv_config = kTestND)
        : params({{onam::kCsvConfig, csv_config},
                  {onam::kLhsIndices, std::move(lhs_indices)},
                  {onam::kRhsIndices, std::move(rhs_indices)},
                  {onam::kEqualNulls, true},
                  {onam::kWeight, weight}}) {}
};

class TestNDVerifying : public ::testing::TestWithParam<NDVerifyingParams> {};

TEST_P(TestNDVerifying, DefaultTest) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto verifier = algos::CreateAndLoadAlgorithm<algos::nd_verifier::NDVerifier>(mp);
    verifier->Execute();
    EXPECT_EQ(verifier->NDHolds(), true);
}

/* Valid NDs:
    0 ->(4) 1
    0 ->(6) 2
    0 ->(4) 3
    0 ->(5) 4
    0 ->(9) 5

    0, 1 ->(3) 3, 5

    ...

*/

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        NDVerifierTestSuite, TestNDVerifying,
        ::testing::Values(
            NDVerifyingParams({0}, {1}, 4),
            NDVerifyingParams({0}, {2}, 6),
            NDVerifyingParams({0}, {3}, 4),
            NDVerifyingParams({0}, {4}, 5),
            NDVerifyingParams({0}, {5}, 9),
            NDVerifyingParams({0, 1}, {3, 5}, 3)
            ));
// clang-format on

}  // namespace tests
