#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/fd/pfdtane/pfd_verifier/pfd_verifier.h"
#include "all_csv_configs.h"
#include "config/indices/type.h"
#include "config/names.h"

namespace tests {
namespace onam = config::names;

namespace {

struct PFDVerifyingParams {
    algos::StdParamsMap params;
    config::ErrorType const expected_error = 0;
    size_t const num_violating_clusters = 0;
    size_t const num_violating_rows = 0;
    std::vector<model::PLI::Cluster> const clusters_violating_pfd;

    PFDVerifyingParams(config::IndicesType lhs_indices, config::IndicesType rhs_indices,
                       config::ErrorMeasureType error_measure, config::ErrorType error,
                       size_t num_violating_clusters, size_t num_violating_rows,
                       std::vector<model::PLI::Cluster> clusters_violating_pfd,
                       CSVConfig const& csv_config)
        : params({{onam::kCsvConfig, csv_config},
                  {onam::kEqualNulls, true},
                  {onam::kLhsIndices, std::move(lhs_indices)},
                  {onam::kRhsIndices, std::move(rhs_indices)},
                  {onam::kErrorMeasure, error_measure},
                  {onam::kError, error}}),
          expected_error(error),
          num_violating_clusters(num_violating_clusters),
          num_violating_rows(num_violating_rows),
          clusters_violating_pfd(std::move(clusters_violating_pfd)) {}
};

class TestPFDVerifying : public ::testing::TestWithParam<PFDVerifyingParams> {};

}  // namespace

TEST_P(TestPFDVerifying, DefaultTest) {
    auto const& p = GetParam();
    auto verifier = algos::CreateAndLoadAlgorithm<algos::PFDVerifier>(p.params);
    double const eps = 0.0001;
    verifier->Execute();
    EXPECT_TRUE(verifier->PFDHolds());
    EXPECT_NEAR(p.expected_error, verifier->GetError(), eps);
    EXPECT_EQ(p.num_violating_clusters, verifier->GetNumViolatingClusters());
    EXPECT_EQ(p.num_violating_rows, verifier->GetNumViolatingRows());
    EXPECT_EQ(p.clusters_violating_pfd, verifier->GetViolatingClusters());
}

INSTANTIATE_TEST_SUITE_P(
        PFDVerifierTestSuite, TestPFDVerifying,
        ::testing::Values(PFDVerifyingParams({2}, {3}, +algos::ErrorMeasure::per_value, 0.0625, 1,
                                             1, {{0, 1}}, kTestFD),
                          PFDVerifyingParams({0, 1}, {4}, +algos::ErrorMeasure::per_value, 0.166667,
                                             2, 2, {{0, 1, 2}, {6, 7, 8}}, kTestFD),
                          PFDVerifyingParams({4}, {5}, +algos::ErrorMeasure::per_value, 0.3334, 4,
                                             4, {{0, 8}, {1, 2}, {3, 4, 5}, {9, 10, 11}}, kTestFD),
                          PFDVerifyingParams({5}, {1}, +algos::ErrorMeasure::per_value, 0.0, 0, 0,
                                             {}, kTestFD),
                          PFDVerifyingParams({2}, {3}, +algos::ErrorMeasure::per_tuple, 0.0834, 1,
                                             1, {{0, 1}}, kTestFD),
                          PFDVerifyingParams({0, 1}, {4}, +algos::ErrorMeasure::per_tuple, 0.1667,
                                             2, 2, {{0, 1, 2}, {6, 7, 8}}, kTestFD),
                          PFDVerifyingParams({4}, {5}, +algos::ErrorMeasure::per_tuple, 0.3334, 4,
                                             4, {{0, 8}, {1, 2}, {3, 4, 5}, {9, 10, 11}}, kTestFD),
                          PFDVerifyingParams({5}, {1}, +algos::ErrorMeasure::per_tuple, 0.0, 0, 0,
                                             {}, kTestFD)));

}  // namespace tests