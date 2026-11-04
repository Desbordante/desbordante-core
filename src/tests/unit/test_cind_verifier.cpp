#include <list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/cind/cind.h"
#include "core/algorithms/cind/cind_algorithm.h"
#include "core/algorithms/cind/cind_verifier/cind_verifier.h"
#include "core/algorithms/cind/condition.h"
#include "core/algorithms/cind/types.h"
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
            {kConditionType, algos::cind::CondType::kGroup},
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

    auto create_and_load = []() {
        algos::CreateAndLoadAlgorithm<algos::cind::CINDVerifier>(algos::StdParamsMap{
                {kCsvConfigs, CSVConfigs{kIndTestEmpty}},
                {kLhsIndices, config::IndicesType{0}},
                {kRhsIndices, config::IndicesType{0}},
        });
    };

    ASSERT_THROW(create_and_load(), std::runtime_error);
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
            CINDVerifierBadConfig({kIndTestTypos}, {{0, 0}, {2, 3}})
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

namespace {

struct RhsConditionExpectation {
    std::vector<std::string> condition;
    double validity;
    double completeness;
    std::size_t violating_clusters;
    std::size_t violating_rows;
};

std::unique_ptr<algos::cind::CINDVerifier> CreateEnDeVerifier(
        std::vector<std::string> const& condition) {
    using namespace config::names;
    return algos::CreateAndLoadAlgorithm<algos::cind::CINDVerifier>(algos::StdParamsMap{
            {kCsvConfigs, CSVConfigs{kTestCINDEn, kTestCINDDe}},
            {kLhsIndices, config::IndicesType{0}},
            {kRhsIndices, config::IndicesType{0}},
            {kCindCondValues, condition},
            {kConditionType, algos::cind::CondType::kGroup},
    });
}

}  // namespace

class TestCINDVerifierRhsCondition : public ::testing::TestWithParam<RhsConditionExpectation> {};

TEST_P(TestCINDVerifierRhsCondition, MatchesExpectedStatistics) {
    RhsConditionExpectation const& p = GetParam();

    auto verifier = CreateEnDeVerifier(p.condition);
    verifier->Execute();

    EXPECT_NEAR(verifier->GetRealValidity(), p.validity, 1e-9);
    EXPECT_NEAR(verifier->GetRealCompleteness(), p.completeness, 1e-9);
    EXPECT_EQ(verifier->GetViolatingClustersCount(), p.violating_clusters);
    EXPECT_EQ(verifier->GetViolatingRowsCount(), p.violating_rows);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        CINDVerifierTestSuite, TestCINDVerifierRhsCondition,
        ::testing::Values(
            RhsConditionExpectation{{"18", "_", "_", "_", "_", "Kap", "_", "_"}, 1.0 / 3, 0.5, 2, 2},
            RhsConditionExpectation{{"18", "_", "_", "_", "_", "_", "LA, Cal", "_"}, 1.0 / 3, 0.5, 2, 2},
            RhsConditionExpectation{{"_", "_", "_", "Athlette", "_", "_", "Queens", "_"}, 1.0, 0.5, 0, 0},
            RhsConditionExpectation{{"_", "_", "_", "Athlette", "_", "_", "Nowhere", "_"}, 0.0, 0.0, 1, 1},
            RhsConditionExpectation{{"18", "_", "_", "_", "_", "_", "_", "_"}, 2.0 / 3, 1.0, 1, 1},
            RhsConditionExpectation{{"18", "_", "_", "_"}, 2.0 / 3, 1.0, 1, 1}
        ));
// clang-format on

TEST(TestCINDVerifierRhsCondition, WrongConditionSizeIsRejected) {
    auto verifier = CreateEnDeVerifier({"18", "_", "_", "_", "_"});
    ASSERT_THROW(verifier->Execute(), std::runtime_error);
}

namespace {

std::list<algos::cind::CIND> MineCinds(algos::cind::AlgoType algo_type,
                                       algos::StdParamsMap extra_params) {
    using namespace config::names;
    algos::StdParamsMap params{
            {kCsvConfigs, CSVConfigs{kTestCINDDe, kTestCINDEn}},
            {kAlgoType, algo_type},
            {kError, config::ErrorType{0.5}},
    };
    params.merge(extra_params);
    auto algo = algos::CreateAndLoadAlgorithm<algos::cind::CindAlgorithm>(params);
    algo->Execute();
    return algo->CINDList();
}

std::unique_ptr<algos::cind::CINDVerifier> CreateVerifierFor(algos::cind::CIND cind,
                                                             algos::cind::CondType cond_type) {
    using namespace config::names;
    return algos::CreateAndLoadAlgorithm<algos::cind::CINDVerifier>(algos::StdParamsMap{
            {kCsvConfigs, CSVConfigs{kTestCINDDe, kTestCINDEn}},
            {kCind, std::optional<algos::cind::CIND>{std::move(cind)}},
            {kConditionType, cond_type},
    });
}

}  // namespace

class TestCINDVerifierMinedInput : public ::testing::TestWithParam<algos::cind::CondType> {};

TEST_P(TestCINDVerifierMinedInput, CinderellaConditionsAreReproduced) {
    using namespace config::names;
    algos::cind::CondType const cond_type = GetParam();

    std::list<algos::cind::CIND> const cinds =
            MineCinds(algos::cind::AlgoType::kCinderella,
                      {{kConditionType, cond_type}, {kValidity, 0.1}, {kCompleteness, 0.4}});
    ASSERT_FALSE(cinds.empty());

    std::size_t checked = 0;
    for (algos::cind::CIND const& cind : cinds) {
        for (algos::cind::Condition const& condition : cind.conditions) {
            algos::cind::CIND single{.ind = cind.ind,
                                     .conditions = {condition},
                                     .conditional_attributes = cind.conditional_attributes};
            auto verifier = CreateVerifierFor(std::move(single), cond_type);
            verifier->Execute();

            EXPECT_NEAR(verifier->GetRealValidity(), condition.validity, 1e-9)
                    << cind.ind.ToLongString() << " " << condition.ToString();
            EXPECT_NEAR(verifier->GetRealCompleteness(), condition.completeness, 1e-9)
                    << cind.ind.ToLongString() << " " << condition.ToString();
            ++checked;
        }
    }
    EXPECT_GT(checked, 0u);
}

INSTANTIATE_TEST_SUITE_P(CINDVerifierTestSuite, TestCINDVerifierMinedInput,
                         ::testing::Values(algos::cind::CondType::kGroup,
                                           algos::cind::CondType::kRow));

TEST(TestCINDVerifierMinedInput, WholeMinedCindIsAccepted) {
    using namespace config::names;

    for (algos::cind::AlgoType const algo_type :
         {algos::cind::AlgoType::kCinderella, algos::cind::AlgoType::kCureCind}) {
        algos::StdParamsMap extra =
                (algo_type == algos::cind::AlgoType::kCureCind)
                        ? algos::StdParamsMap{{kCindMinSupport, 2u}}
                        : algos::StdParamsMap{{kValidity, 0.1}, {kCompleteness, 0.4}};
        std::list<algos::cind::CIND> const cinds = MineCinds(algo_type, std::move(extra));
        ASSERT_FALSE(cinds.empty());

        for (algos::cind::CIND const& cind : cinds) {
            auto verifier = CreateVerifierFor(cind, algos::cind::CondType::kGroup);
            ASSERT_NO_THROW(verifier->Execute()) << cind.ToString();

            EXPECT_LE(verifier->GetIncludedSupportingBaskets(), verifier->GetSupportingBaskets());
            EXPECT_LE(verifier->GetIncludedSupportingBaskets(),
                      verifier->GetIncludedBasketsTotal());
            if (!cind.conditions.empty()) {
                EXPECT_GE(verifier->GetRealValidity(), 0.0) << cind.ToString();
            }
        }
    }
}

TEST(TestCINDVerifierMinedInput, CindWithWrongConditionSizeIsRejected) {
    using namespace config::names;

    std::list<algos::cind::CIND> const cinds =
            MineCinds(algos::cind::AlgoType::kCinderella, {{kValidity, 0.1}, {kCompleteness, 0.4}});
    ASSERT_FALSE(cinds.empty());

    algos::cind::CIND const& mined = cinds.front();
    algos::cind::CIND bad{.ind = mined.ind,
                          .conditions = {algos::cind::Condition{{"18"}, 1.0, 1.0}},
                          .conditional_attributes = mined.conditional_attributes};

    auto verifier = CreateVerifierFor(std::move(bad), algos::cind::CondType::kGroup);
    ASSERT_THROW(verifier->Execute(), std::runtime_error);
}

TEST(TestCINDVerifierMinedInput, CindReferringToMissingTableIsRejected) {
    using namespace config::names;

    std::list<algos::cind::CIND> const cinds =
            MineCinds(algos::cind::AlgoType::kCinderella, {{kValidity, 0.1}, {kCompleteness, 0.4}});
    ASSERT_FALSE(cinds.empty());

    auto create = [&]() {
        algos::CreateAndLoadAlgorithm<algos::cind::CINDVerifier>(algos::StdParamsMap{
                {kCsvConfigs, CSVConfigs{kTestCINDDe}},
                {kCind, std::optional<algos::cind::CIND>{cinds.back()}},
        });
    };
    ASSERT_THROW(create(), config::ConfigurationError);
}

}  // namespace tests
