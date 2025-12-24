#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/pfd_verifier/pfd_verifier.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

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
                       config::PfdErrorMeasureType error_measure, config::ErrorType error,
                       size_t num_violating_clusters, size_t num_violating_rows,
                       std::vector<model::PLI::Cluster> clusters_violating_pfd,
                       CSVConfig const& csv_config)
        : params({
                  {onam::kCsvConfig, csv_config},
                  {onam::kEqualNulls, true},
                  {onam::kLhsIndices, std::move(lhs_indices)},
                  {onam::kRhsIndices, std::move(rhs_indices)},
                  {onam::kPfdErrorMeasure, error_measure},
          }),
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
    EXPECT_NEAR(p.expected_error, verifier->GetError(), eps);
    EXPECT_EQ(p.num_violating_clusters, verifier->GetNumViolatingClusters());
    EXPECT_EQ(p.num_violating_rows, verifier->GetNumViolatingRows());
    EXPECT_EQ(p.clusters_violating_pfd, verifier->GetViolatingClusters());
}

INSTANTIATE_TEST_SUITE_P(
        PFDVerifierTestSuite, TestPFDVerifying,
        ::testing::Values(PFDVerifyingParams({2}, {3}, algos::PfdErrorMeasure::kPerValue, 0.0625, 1,
                                             1, {{0, 1}}, kTestFD),
                          PFDVerifyingParams({0, 1}, {4}, algos::PfdErrorMeasure::kPerValue,
                                             0.166667, 2, 2, {{0, 1, 2}, {6, 7, 8}}, kTestFD),
                          PFDVerifyingParams({4}, {5}, algos::PfdErrorMeasure::kPerValue, 0.3334, 4,
                                             4, {{0, 8}, {1, 2}, {3, 4, 5}, {9, 10, 11}}, kTestFD),
                          PFDVerifyingParams({5}, {1}, algos::PfdErrorMeasure::kPerValue, 0.0, 0, 0,
                                             {}, kTestFD),
                          PFDVerifyingParams({2}, {3}, algos::PfdErrorMeasure::kPerTuple, 0.0834, 1,
                                             1, {{0, 1}}, kTestFD),
                          PFDVerifyingParams({0, 1}, {4}, algos::PfdErrorMeasure::kPerTuple, 0.1667,
                                             2, 2, {{0, 1, 2}, {6, 7, 8}}, kTestFD),
                          PFDVerifyingParams({4}, {5}, algos::PfdErrorMeasure::kPerTuple, 0.3334, 4,
                                             4, {{0, 8}, {1, 2}, {3, 4, 5}, {9, 10, 11}}, kTestFD),
                          PFDVerifyingParams({5}, {1}, algos::PfdErrorMeasure::kPerTuple, 0.0, 0, 0,
                                             {}, kTestFD)));

}  // namespace tests
