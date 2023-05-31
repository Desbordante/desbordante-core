#include <algorithm>
#include <filesystem>
#include <memory>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/ucc_verifier/ucc_verifier.h"
#include "util/config/indices/type.h"
#include "datasets.h"

namespace tests {
namespace onam = util::config::names;

struct UCCVerifyingParams {
    algos::StdParamsMap params;
    size_t const num_error_clusters = 0;
    size_t const num_error_rows = 0;

    UCCVerifyingParams(util::config::IndicesType column_indices,
                      size_t const num_error_clusters = 0,
                      size_t const num_error_rows = 0,
                      char const* dataset = "TestUCCVerifier.csv", char const separator = ',',
                      bool const has_header = true)
        : params({{onam::kIndices, std::move(column_indices)},
                  {onam::kData, test_data_dir / dataset},
                  {onam::kSeparator, separator},
                  {onam::kHasHeader, has_header},
                  {onam::kEqualNulls, true}}),
          num_error_clusters(num_error_clusters),
          num_error_rows(num_error_rows) {}
};

class TestUCCVerifying : public ::testing::TestWithParam<UCCVerifyingParams> {};

TEST_P(TestUCCVerifying, DefaultTest) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto verifier = algos::CreateAndLoadAlgorithm<algos::ucc_verifier::UCCVerifier>(mp);
    verifier->Execute();
    EXPECT_EQ(verifier->UCCHolds(), p.num_error_clusters == 0);
    EXPECT_EQ(verifier->GetNumErrorRows(), p.num_error_rows);
    EXPECT_EQ(verifier->GetNumErrorClusters(), p.num_error_clusters);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        UCCVerifierTestSuite, TestUCCVerifying,
        ::testing::Values(
            UCCVerifyingParams({0}, 1, 5),
            UCCVerifyingParams({0, 1}, 1, 4),
            UCCVerifyingParams({0, 1, 2}, 1, 4),
            UCCVerifyingParams({0, 1, 2, 3}, 2, 4),
            UCCVerifyingParams({0, 1, 2, 3, 4}, 0, 0),
            UCCVerifyingParams({0, 1, 2, 3, 4, 5}, 0, 0)
            ));
// clang-format on

}  // namespace tests