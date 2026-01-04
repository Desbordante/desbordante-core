#include <memory>
#include <optional>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/cind/cind_verifier/cind_verifier.h"
#include "core/algorithms/ind/ind_verifier/ind_verifier.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {

struct CINDVerifierTestConfig {
    CSVConfigs csv_configs;
    algos::INDVerifier::RawIND ind;

    CINDVerifierTestConfig(CSVConfigs const& csv_configs, algos::INDVerifier::RawIND&& ind)
        : csv_configs(csv_configs), ind(std::move(ind)) {}
};

namespace {

static std::unique_ptr<algos::INDVerifier> CreateINDVerifier(CINDVerifierTestConfig const& cfg) {
    using namespace config::names;
    return algos::CreateAndLoadAlgorithm<algos::INDVerifier>(algos::StdParamsMap{
            {kCsvConfigs, cfg.csv_configs},
            {kLhsIndices, cfg.ind.lhs},
            {kRhsIndices, cfg.ind.rhs},
    });
}

static std::unique_ptr<algos::cind::CINDVerifier> CreateCINDVerifier(
        CINDVerifierTestConfig const& cfg) {
    using namespace config::names;
    return algos::CreateAndLoadAlgorithm<algos::cind::CINDVerifier>(algos::StdParamsMap{
            {kCsvConfigs, cfg.csv_configs},
            {kLhsIndices, cfg.ind.lhs},
            {kRhsIndices, cfg.ind.rhs},
    });
}

}  // namespace

class TestCINDVerifier : public ::testing::TestWithParam<CINDVerifierTestConfig> {};

TEST_P(TestCINDVerifier, MatchesINDVerifierForEmptyCondition) {
    CINDVerifierTestConfig const& cfg = GetParam();

    auto ind = CreateINDVerifier(cfg);
    ind->Execute();

    auto cind = CreateCINDVerifier(cfg);
    cind->Execute();

    EXPECT_EQ(cind->Holds(), ind->Holds());
    EXPECT_NEAR(cind->GetRealValidity(), 1.0 - ind->GetError(), 1e-12);

    if (cind->GetIncludedBasketsTotal() > 0) {
        EXPECT_DOUBLE_EQ(cind->GetRealCompleteness(), 1.0);
    } else {
        EXPECT_DOUBLE_EQ(cind->GetRealCompleteness(), 0.0);
    }

    EXPECT_EQ(cind->GetViolatingClustersCount(), ind->GetViolatingClustersCount());
    EXPECT_EQ(cind->GetViolatingRowsCount(), ind->GetViolatingRowsCount());

    if (!ind->Holds()) {
        EXPECT_GT(cind->GetViolatingClustersCount(), 0u);
        EXPECT_GT(cind->GetViolatingRowsCount(), 0u);
    }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        CINDVerifierTestSuite, TestCINDVerifier,
        ::testing::Values(
            CINDVerifierTestConfig({kIndTest3aryInds}, {{3}, {0}}),
            CINDVerifierTestConfig({kIndTest3aryInds}, {{4}, {1}}),
            CINDVerifierTestConfig({kIndTest3aryInds}, {{1}, {4}}),

            CINDVerifierTestConfig({kIndTestTableFirst, kIndTestTableSecond}, {{0, 1, 2, 3}, {0, 1, 3, 4}}),
            CINDVerifierTestConfig({kIndTestTableSecond, kIndTestTableFirst}, {{0, 1, 3, 4}, {0, 1, 2, 3}}),

            CINDVerifierTestConfig({kIndTestTypos}, {{0}, {1}}),
            CINDVerifierTestConfig({kIndTestTypos}, {{0}, {2}}),
            CINDVerifierTestConfig({kIndTestTypos}, {{1}, {3}}),
            CINDVerifierTestConfig({kIndTestTypos}, {{0, 1}, {2, 3}})
        ));
// clang-format on

TEST(TestCINDVerifierRuntimeError, TestEmptyTable) {
    using namespace config::names;

    auto verifier = algos::CreateAndLoadAlgorithm<algos::cind::CINDVerifier>(algos::StdParamsMap{
            {kCsvConfigs, CSVConfigs{kIndTestEmpty}},
            {kLhsIndices, config::IndicesType{0}},
            {kRhsIndices, config::IndicesType{0}},
    });

    ASSERT_THROW(verifier->Execute(), std::runtime_error);
}

struct CINDVerifierBadConfig {
    CSVConfigs csv_configs;
    algos::INDVerifier::RawIND ind;

    CINDVerifierBadConfig(CSVConfigs const& csv_configs, algos::INDVerifier::RawIND&& ind)
        : csv_configs(csv_configs), ind(std::move(ind)) {}
};

class TestCINDVerifierConfigurationError : public ::testing::TestWithParam<CINDVerifierBadConfig> {
};

TEST_P(TestCINDVerifierConfigurationError, TestIncorrectIndices) {
    using namespace config::names;

    auto const& p = GetParam();
    auto create_and_execute = [&]() {
        auto verifier =
                algos::CreateAndLoadAlgorithm<algos::cind::CINDVerifier>(algos::StdParamsMap{
                        {kCsvConfigs, p.csv_configs},
                        {kLhsIndices, p.ind.lhs},
                        {kRhsIndices, p.ind.rhs},
                });
        verifier->Execute();
    };

    ASSERT_THROW(create_and_execute(), config::ConfigurationError);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        CINDVerifierFailureTestSuite, TestCINDVerifierConfigurationError,
        ::testing::Values(
            CINDVerifierBadConfig({kIndTestTypos}, {{0}, {5}}),
            CINDVerifierBadConfig({kIndTestTypos}, {{6}, {7}}),
            CINDVerifierBadConfig({kIndTestTypos}, {{0, 1}, {2, 3, 4}}),
            CINDVerifierBadConfig({kIndTestTypos}, {{0, 0}, {2, 3}}),
            CINDVerifierBadConfig({kIndTestTypos}, {{0, 0}, {2}})
        ));
// clang-format on

TEST_P(TestCINDVerifier, ViolatingClustersAreConsistent) {
    CINDVerifierTestConfig const& cfg = GetParam();

    auto cind = CreateCINDVerifier(cfg);
    cind->Execute();

    auto const& clusters = cind->GetViolatingClusters();

    std::size_t sum_violating_rows = 0;

    for (auto const& vc : clusters) {
        EXPECT_FALSE(vc.basket_rows.empty());
        EXPECT_FALSE(vc.violating_rows.empty());

        std::unordered_set<model::TupleIndex> basket_set(vc.basket_rows.begin(),
                                                         vc.basket_rows.end());

        for (auto rid : vc.violating_rows) {
            EXPECT_TRUE(basket_set.contains(rid));
        }

        sum_violating_rows += vc.violating_rows.size();
    }

    EXPECT_EQ(sum_violating_rows, cind->GetViolatingRowsCount());
}

TEST_P(TestCINDVerifier, SupportingBasketsAndIncludedSupportBounds) {
    CINDVerifierTestConfig const& cfg = GetParam();

    auto cind = CreateCINDVerifier(cfg);
    cind->Execute();

    EXPECT_LE(cind->GetIncludedSupportingBaskets(), cind->GetSupportingBaskets());
    EXPECT_LE(cind->GetIncludedSupportingBaskets(), cind->GetIncludedBasketsTotal());
}

}  // namespace tests
