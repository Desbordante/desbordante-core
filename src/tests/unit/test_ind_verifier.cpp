#include <memory>

#include <gtest/gtest.h>

#include "algo_factory.h"
#include "all_csv_configs.h"
#include "config/names.h"
#include "csv_config_util.h"
#include "error/type.h"
#include "ind/ind_verifier/ind_verifier.h"

namespace tests {

struct INDVerifierErrorInfo {
    unsigned int num_violating_rows;
    unsigned int num_violating_clusters;
    config::ErrorType error;
};

struct INDVerifierTestConfig {
    CSVConfigs csv_configs;
    /* IND to check */
    algos::INDVerifier::RawIND ind;
    /* `std::nullopt` iff IND holds */
    std::optional<INDVerifierErrorInfo> error_opt{};
};

namespace {
static std::unique_ptr<algos::INDVerifier> CreateINDVerfier(INDVerifierTestConfig const& config) {
    using namespace config::names;
    return algos::CreateAndLoadAlgorithm<algos::INDVerifier>(algos::StdParamsMap{
            {kCsvConfigs, config.csv_configs},
            {kRhsIndices, config.ind.rhs},
            {kLhsIndices, config.ind.lhs},
    });
}
}  // namespace

class TestINDVerifier : public ::testing::TestWithParam<INDVerifierTestConfig> {};

TEST_P(TestINDVerifier, DefaultTest) {
    using namespace config::names;

    INDVerifierTestConfig const& config = GetParam();
    std::unique_ptr<algos::INDVerifier> verifier = CreateINDVerfier(config);
    verifier->Execute();

    static INDVerifierErrorInfo const kINDHolds{
            .num_violating_rows = 0,
            .num_violating_clusters = 0,
            .error = config::ErrorType{0.0},
    };

    EXPECT_NE(verifier->Holds(), config.error_opt.has_value());
    INDVerifierErrorInfo const& error_info =
            config.error_opt ? config.error_opt.value() : kINDHolds;

    EXPECT_DOUBLE_EQ(verifier->GetError(), error_info.error);
    EXPECT_EQ(verifier->GetViolatingClustersCount(), error_info.num_violating_clusters);
    EXPECT_EQ(verifier->GetViolatingRowsCount(), error_info.num_violating_rows);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        INDVerifierTestSuite, TestINDVerifier,
        ::testing::Values(
            INDVerifierTestConfig({kIndTest3aryInds}, {{3}, {0}}),
            INDVerifierTestConfig({kIndTest3aryInds}, {{4}, {1}}),
            INDVerifierTestConfig({kIndTest3aryInds}, {{1}, {4}}, INDVerifierErrorInfo{.num_violating_rows = 1, .num_violating_clusters = 1, .error = config::ErrorType{1}/3}),
            INDVerifierTestConfig({kIndTestTableFirst, kIndTestTableSecond}, {{0, 1, 2, 3}, {0, 1, 3, 4}}, std::nullopt),
            INDVerifierTestConfig({kIndTestTableSecond, kIndTestTableFirst}, {{0, 1, 3, 4}, {0, 1, 2, 3}}, std::nullopt),
            INDVerifierTestConfig({kIndTestTypos}, {{0}, {1}}, INDVerifierErrorInfo{.num_violating_rows = 8, .num_violating_clusters = 6, .error = config::ErrorType{6}/6}),
            INDVerifierTestConfig({kIndTestTypos}, {{0}, {2}}, INDVerifierErrorInfo{.num_violating_rows = 3, .num_violating_clusters = 2, .error = config::ErrorType{2}/6}),
            INDVerifierTestConfig({kIndTestTypos}, {{1}, {3}}, INDVerifierErrorInfo{.num_violating_rows = 3, .num_violating_clusters = 2, .error = config::ErrorType{2}/6}),
            INDVerifierTestConfig({kIndTestTypos}, {{0,1}, {2,3}}, INDVerifierErrorInfo{.num_violating_rows = 3, .num_violating_clusters = 2, .error = config::ErrorType{2}/6}),
            INDVerifierTestConfig({kIndTestTypos}, {{2}, {0}}, INDVerifierErrorInfo{.num_violating_rows = 4, .num_violating_clusters = 4, .error = config::ErrorType{4}/8}),
            INDVerifierTestConfig({kIndTestTypos}, {{4}, {0}}, INDVerifierErrorInfo{.num_violating_rows = 5, .num_violating_clusters = 2, .error = config::ErrorType{2}/5}),
            INDVerifierTestConfig({kIndTestTypos}, {{4}, {1}}, INDVerifierErrorInfo{.num_violating_rows = 3, .num_violating_clusters = 3, .error = config::ErrorType{3}/5}),
            INDVerifierTestConfig({kIndTestTypos}, {{4}, {2}}, INDVerifierErrorInfo{.num_violating_rows = 7, .num_violating_clusters = 4, .error = config::ErrorType{4}/5}),
            INDVerifierTestConfig({kIndTestTypos}, {{4}, {3}}, INDVerifierErrorInfo{.num_violating_rows = 4, .num_violating_clusters = 4, .error = config::ErrorType{4}/5})
            ));
// clang-format on

class TestINDVerifierRuntimeError : public ::testing::TestWithParam<INDVerifierTestConfig> {};

TEST_F(TestINDVerifierRuntimeError, TestEmptyTable) {
    INDVerifierTestConfig const& config = INDVerifierTestConfig({kIndTestEmpty}, {{0}, {0}});
    std::unique_ptr<algos::INDVerifier> verifier = CreateINDVerfier(config);
    ASSERT_THROW(verifier->Execute(), std::runtime_error);
}

class TestINDVerifierConfigurationError : public ::testing::TestWithParam<INDVerifierTestConfig> {};

TEST_P(TestINDVerifierConfigurationError, TestIncorrectIndices) {
    auto const create_and_execute = [&](INDVerifierTestConfig const& config) {
        std::unique_ptr<algos::INDVerifier> verifier = CreateINDVerfier(config);
        verifier->Execute();
    };
    ASSERT_THROW(create_and_execute(GetParam()), config::ConfigurationError);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        INDVerifierFailureTestSuite, TestINDVerifierConfigurationError,
        ::testing::Values(
            INDVerifierTestConfig({kIndTestTypos}, {{0}, {5}}),
            INDVerifierTestConfig({kIndTestTypos}, {{6}, {7}}),
            INDVerifierTestConfig({kIndTestTypos}, {{0, 1}, {2, 3, 4}}),
            INDVerifierTestConfig({kIndTestTypos}, {{0, 0}, {2, 3}}),
            INDVerifierTestConfig({kIndTestTypos}, {{0, 0}, {2}})
            ));
// clang-format on

}  // namespace tests
