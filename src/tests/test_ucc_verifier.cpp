#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/ucc/ucc_verifier/ucc_verifier.h"
#include "config/indices/type.h"
#include "datasets.h"

namespace tests {
namespace onam = config::names;

class UCCVerifierSimpleParams {
private:
    algos::StdParamsMap params_map_;
    size_t num_clusters_violating_ucc_ = 0;
    size_t num_rows_violating_ucc_ = 0;
    std::vector<model::PLI::Cluster> clusters_violating_ucc_;

public:
    UCCVerifierSimpleParams(config::IndicesType column_indices,
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

class TestUCCVerifierSimple : public ::testing::TestWithParam<UCCVerifierSimpleParams> {};

class UCCVerifierWithHyUCCParams {
private:
    algos::StdParamsMap ucc_verifier_params_map_;
    algos::StdParamsMap hyucc_params_map_;

public:
    explicit UCCVerifierWithHyUCCParams(std::string_view dataset, char const separator = ',',
                                        bool const has_header = true)
        : ucc_verifier_params_map_({{onam::kCsvPath, test_data_dir / dataset},
                                    {onam::kSeparator, separator},
                                    {onam::kHasHeader, has_header},
                                    {onam::kEqualNulls, true}}),
          hyucc_params_map_({{onam::kThreads, static_cast<config::ThreadNumType>(1)},
                             {onam::kCsvPath, test_data_dir / dataset},
                             {onam::kSeparator, separator},
                             {onam::kHasHeader, has_header},
                             {onam::kEqualNulls, true}}) {}

    algos::StdParamsMap GetUCCVerifierParamsMap() const {
        return ucc_verifier_params_map_;
    }

    algos::StdParamsMap GetHyUCCParamsMap() const {
        return hyucc_params_map_;
    }
};

class TestUCCVerifierWithHyUCC : public ::testing::TestWithParam<UCCVerifierWithHyUCCParams> {};

TEST_P(TestUCCVerifierSimple, DefaultTest) {
    UCCVerifierSimpleParams const& p(GetParam());
    auto verifier = algos::CreateAndLoadAlgorithm<algos::UCCVerifier>(p.GetParamsMap());
    verifier->Execute();
    EXPECT_EQ(verifier->UCCHolds(), p.GetExpectedNumClustersViolatingUCC() == 0);
    EXPECT_EQ(verifier->GetNumRowsViolatingUCC(), p.GetExpectedNumRowsViolatingUCC());
    EXPECT_EQ(verifier->GetNumClustersViolatingUCC(), p.GetExpectedNumClustersViolatingUCC());
    EXPECT_EQ(verifier->GetClustersViolatingUCC(), p.GetExpectedClustersViolatingUCC());
}

INSTANTIATE_TEST_SUITE_P(
        UCCVerifierSimpleTestSuite, TestUCCVerifierSimple,
        ::testing::Values(UCCVerifierSimpleParams({0}, 1, 12,
                                                  {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
                                                  "TestFD.csv"),
                          UCCVerifierSimpleParams({0, 1}, 4, 12,
                                                  {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {9, 10, 11}},
                                                  "TestFD.csv"),
                          UCCVerifierSimpleParams({0, 1, 2}, 4, 8,
                                                  {{0, 1}, {3, 4}, {6, 7}, {9, 10}}, "TestFD.csv"),
                          UCCVerifierSimpleParams({0, 1, 2, 3, 4, 5}, 3, 6,
                                                  {{3, 4}, {6, 7}, {9, 10}}, "TestFD.csv"),
                          UCCVerifierSimpleParams({0}, 0, 0, {}, "TestWide.csv"),
                          UCCVerifierSimpleParams({0, 1, 2, 3, 4}, 0, 0, {}, "TestWide.csv")));

TEST_P(TestUCCVerifierWithHyUCC, TestWithHyUCC) {
    UCCVerifierWithHyUCCParams const& p(GetParam());

    auto hyucc = algos::CreateAndLoadAlgorithm<algos::HyUCC>(p.GetHyUCCParamsMap());
    hyucc->Execute();
    std::list<model::UCC> const& mined_uccs = hyucc->UCCList();

    // run ucc_verifier on each UCC from mined_ucc (UCC must hold)
    for (auto const& current_ucc : mined_uccs) {
        algos::StdParamsMap ucc_verifier_params_map = p.GetUCCVerifierParamsMap();
        ucc_verifier_params_map.insert({onam::kUCCIndices, current_ucc.GetColumnIndicesAsVector()});
        auto verifier = algos::CreateAndLoadAlgorithm<algos::UCCVerifier>(
                std::move(ucc_verifier_params_map));
        verifier->Execute();
        EXPECT_TRUE(verifier->UCCHolds());
    }

    // Сases of prevent false negative triggering
    // run ucc_verifier on each UCC from mined_ucc with one column index removed
    // (UCC must not hold because HyUCC returns minimal UCCs)
    for (auto const& current_ucc : mined_uccs) {
        algos::StdParamsMap ucc_verifier_params_map = p.GetUCCVerifierParamsMap();
        std::vector<unsigned int> current_ucc_vec = current_ucc.GetColumnIndicesAsVector();
        if (current_ucc_vec.size() < 2) {
            continue;
        }
        current_ucc_vec.erase(--current_ucc_vec.end());
        ucc_verifier_params_map.insert({onam::kUCCIndices, std::move(current_ucc_vec)});
        auto verifier = algos::CreateAndLoadAlgorithm<algos::UCCVerifier>(
                std::move(ucc_verifier_params_map));
        verifier->Execute();
        EXPECT_FALSE(verifier->UCCHolds());
    }
}

INSTANTIATE_TEST_SUITE_P(UCCVerifierWithHyUCCTestSuite, TestUCCVerifierWithHyUCC,
                         ::testing::Values(UCCVerifierWithHyUCCParams("abalone.csv", ',', false),
                                           UCCVerifierWithHyUCCParams("breast_cancer.csv"),
                                           UCCVerifierWithHyUCCParams("CIPublicHighway10k.csv")));
}  // namespace tests
