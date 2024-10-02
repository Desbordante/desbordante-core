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
    std::optional<INDVerifierErrorInfo> error_opt;
};

class TestINDVerifier : public ::testing::TestWithParam<INDVerifierTestConfig> {};

TEST_P(TestINDVerifier, DefaultTest) {
    using namespace config::names;

    auto const& config = GetParam();

    auto verifier = algos::CreateAndLoadAlgorithm<algos::INDVerifier>(algos::StdParamsMap{
            {kCsvConfigs, config.csv_configs},
            {kLhsIndices, config.ind.lhs},
            {kRhsIndices, config.ind.rhs},
    });

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
    EXPECT_EQ(verifier->GetViolatingClusters().size(), error_info.num_violating_clusters);
    EXPECT_EQ(verifier->GetViolatingRowsCount(), error_info.num_violating_rows);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        INDVerifierTestSuite, TestINDVerifier,
        ::testing::Values(
            INDVerifierTestConfig({kIndTest3aryInds}, {{3},{0}}, std::nullopt),
            INDVerifierTestConfig({kIndTest3aryInds}, {{4},{1}}, std::nullopt),
            INDVerifierTestConfig({kIndTest3aryInds}, {{1},{4}}, INDVerifierErrorInfo{.num_violating_rows = 1, .num_violating_clusters = 1, .error = config::ErrorType{1}/3}),
            INDVerifierTestConfig({kIndTestTableFirst, kIndTestTableSecond}, {{0, 1, 2, 3},{0, 1, 3, 4}}, std::nullopt),
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

}  // namespace tests
