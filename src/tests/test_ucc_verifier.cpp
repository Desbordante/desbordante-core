#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/ucc/ucc_verifier/ucc_verifier.h"
#include "config/indices/type.h"
#include "datasets.h"

namespace tests {
namespace onam = config::names;

class UCCVerifyingSimpleParams {
private:
    algos::StdParamsMap params_map_;
    size_t num_clusters_violating_ucc_ = 0;
    size_t num_rows_violating_ucc_ = 0;
    std::vector<model::PLI::Cluster> clusters_violating_ucc_;

public:
    UCCVerifyingSimpleParams(config::IndicesType column_indices,
                             size_t const num_clusters_violating_ucc,
                             size_t const num_rows_violating_ucc,
                             std::vector<model::PLI::Cluster> clusters_violating_ucc,
                             std::string_view dataset, char const separator = ',',
                             bool const has_header = true)
        : params_map_({{onam::kUCCIndices, std::move(column_indices)},
                       {onam::kCsvPath, test_data_dir / dataset},
                       {onam::kSeparator, separator},
                       {onam::kHasHeader, has_header},
                       {onam::kEqualNulls, true}}),
          num_clusters_violating_ucc_(num_clusters_violating_ucc),
          num_rows_violating_ucc_(num_rows_violating_ucc),
          clusters_violating_ucc_(std::move(clusters_violating_ucc)) {}

    algos::StdParamsMap GetParamsMap() const {
        return params_map_;
    }

    size_t GetExpectedNumClustersViolatingUCC() const {
        return num_clusters_violating_ucc_;
    }

    size_t GetExpectedNumRowsViolatingUCC() const {
        return num_rows_violating_ucc_;
    }

    std::vector<model::PLI::Cluster> const& GetExpectedClustersViolatingUCC() const {
        return clusters_violating_ucc_;
    }
};

class TestUCCVerifyingSimple : public ::testing::TestWithParam<UCCVerifyingSimpleParams> {};

TEST_P(TestUCCVerifyingSimple, DefaultTest) {
    UCCVerifyingSimpleParams const& p(GetParam());
    auto verifier = algos::CreateAndLoadAlgorithm<algos::UCCVerifier>(p.GetParamsMap());
    verifier->Execute();
    EXPECT_EQ(verifier->UCCHolds(), p.GetExpectedNumClustersViolatingUCC() == 0);
    EXPECT_EQ(verifier->GetNumRowsViolatingUCC(), p.GetExpectedNumRowsViolatingUCC());
    EXPECT_EQ(verifier->GetNumClustersViolatingUCC(), p.GetExpectedNumClustersViolatingUCC());
    EXPECT_EQ(verifier->GetClustersViolatingUCC(), p.GetExpectedClustersViolatingUCC());
}

INSTANTIATE_TEST_SUITE_P(
        UCCVerifierSimpleTestSuite, TestUCCVerifyingSimple,
        ::testing::Values(UCCVerifyingSimpleParams({0}, 1, 12,
                                                   {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
                                                   "TestFD.csv"),
                          UCCVerifyingSimpleParams({0, 1}, 4, 12,
                                                   {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {9, 10, 11}},
                                                   "TestFD.csv"),
                          UCCVerifyingSimpleParams({0, 1, 2}, 4, 8,
                                                   {{0, 1}, {3, 4}, {6, 7}, {9, 10}}, "TestFD.csv"),
                          UCCVerifyingSimpleParams({0, 1, 2, 3, 4, 5}, 3, 6,
                                                   {{3, 4}, {6, 7}, {9, 10}}, "TestFD.csv"),
                          UCCVerifyingSimpleParams({0}, 0, 0, {}, "TestWide.csv"),
                          UCCVerifyingSimpleParams({0, 1, 2, 3, 4}, 0, 0, {}, "TestWide.csv")));
}  // namespace tests
