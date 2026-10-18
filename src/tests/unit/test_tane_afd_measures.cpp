#include <algorithm>
#include <cmath>

#include <gtest/gtest.h>

#include "test_tane_afd_util.h"

namespace tests {
namespace onam = config::names;

struct TaneMiningParams {
    unsigned int result_hash;
    config::ErrorType error;
    algos::AfdErrorMeasure error_measure;
    CSVConfig csv_config;
};

struct TaneNewMeasureParams {
    algos::AfdErrorMeasure measure;
    config::ErrorType expected_error;
    config::ErrorType zeroary_error;
};

class TestTaneAfdMeasuresMining : public ::testing::TestWithParam<TaneMiningParams> {};

class TestTaneNewAfdMeasures : public ::testing::TestWithParam<TaneNewMeasureParams> {};

TEST_P(TestTaneAfdMeasuresMining, DefaultTest) {
    auto const& p = GetParam();
    algos::StdParamsMap params{{onam::kCsvConfig, p.csv_config},
                               {onam::kError, p.error},
                               {onam::kAfdErrorMeasure, p.error_measure}};
    auto tane = algos::CreateAndLoadAlgorithm<algos::Tane>(params);
    tane->Execute();
    EXPECT_EQ(p.result_hash, tane->Fletcher16());
}

TEST_P(TestTaneNewAfdMeasures, DiscoverExpectedSmallDependency) {
    auto const& p = GetParam();
    auto tane = RunTane(p.measure, p.expected_error + kTaneTolerance);
    ::AFD const* afd = FindAfd(*tane, {0}, 1);
    ASSERT_NE(afd, nullptr);
    EXPECT_NEAR(afd->GetThreshold(), p.expected_error, kTaneTolerance);
}

TEST_P(TestTaneNewAfdMeasures, PreserveMinimalityWithExactlyRedundantLhs) {
    auto const& p = GetParam();
    auto tane = RunTane(p.measure, p.expected_error + kTaneTolerance, 2);
    ASSERT_NE(FindAfd(*tane, {4}, 1), nullptr);
    EXPECT_EQ(FindAfd(*tane, {4, 5}, 1), nullptr);
}

TEST_P(TestTaneNewAfdMeasures, ExactDependenciesAndKeysHaveZeroError) {
    auto tane = RunTane(GetParam().measure, 0.0);
    ::AFD const* exact = FindAfd(*tane, {4}, 5);
    ASSERT_NE(exact, nullptr);
    EXPECT_DOUBLE_EQ(exact->GetThreshold(), 0.0);
    ::AFD const* key = FindAfd(*tane, {6}, 1);
    ASSERT_NE(key, nullptr);
    EXPECT_DOUBLE_EQ(key->GetThreshold(), 0.0);
}

TEST_P(TestTaneNewAfdMeasures, NonExactDependencyIsRejectedAtZeroThreshold) {
    auto tane = RunTane(GetParam().measure, 0.0);
    EXPECT_EQ(FindAfd(*tane, {0}, 1), nullptr);
}

TEST_P(TestTaneNewAfdMeasures, CalculateZeroAryErrors) {
    auto const& p = GetParam();
    auto below_threshold = RunTane(p.measure, p.zeroary_error - kTaneTolerance, 0);
    EXPECT_EQ(FindAfd(*below_threshold, {}, 1), nullptr);
    ::AFD const* constant = FindAfd(*below_threshold, {}, 7);
    ASSERT_NE(constant, nullptr);
    EXPECT_DOUBLE_EQ(constant->GetThreshold(), 0.0);
    auto at_threshold = RunTane(p.measure, std::min(p.zeroary_error + kTaneTolerance, 1.0), 0);
    ::AFD const* nonconstant = FindAfd(*at_threshold, {}, 1);
    ASSERT_NE(nonconstant, nullptr);
    EXPECT_NEAR(nonconstant->GetThreshold(), p.zeroary_error, kTaneTolerance);
}

INSTANTIATE_TEST_SUITE_P(
        NewAfdMeasuresTaneSuite, TestTaneNewAfdMeasures,
        ::testing::Values(
                TaneNewMeasureParams{algos::AfdErrorMeasure::kG1S,
                                     (std::log(3.0) - 2.0 / 3.0 * std::log(2.0)) / 2.0,
                                     std::log(3.0) - 2.0 / 3.0 * std::log(2.0)},
                TaneNewMeasureParams{algos::AfdErrorMeasure::kRfiPlus, 0.7, 1.0},
                TaneNewMeasureParams{algos::AfdErrorMeasure::kRfiPrimePlus, 0.625, 1.0},
                TaneNewMeasureParams{algos::AfdErrorMeasure::kSfi, 0.78475748010537394, 1.0},
                TaneNewMeasureParams{algos::AfdErrorMeasure::kG1Prime, 2.0 / 15.0, 8.0 / 15.0},
                TaneNewMeasureParams{algos::AfdErrorMeasure::kG3Prime, 1.0 / 4.0, 2.0 / 5.0}));

INSTANTIATE_TEST_SUITE_P(
        AfdMeasuresTaneMiningSuite, TestTaneAfdMeasuresMining,
        ::testing::Values(
                TaneMiningParams(29253, 0.3, algos::AfdErrorMeasure::kPdep, kTestFD),
                TaneMiningParams(19266, 0.174, algos::AfdErrorMeasure::kPdep, kIris),
                TaneMiningParams(18528, 0.1, algos::AfdErrorMeasure::kPdep, kIris),
                TaneMiningParams(53657, 0.15, algos::AfdErrorMeasure::kPdep, kNeighbors10k),
                TaneMiningParams(44088, 0.21, algos::AfdErrorMeasure::kPdep, kNeighbors10k),
                TaneMiningParams(21731, 0.01, algos::AfdErrorMeasure::kTau, kTestFD),
                TaneMiningParams(11680, 0.1, algos::AfdErrorMeasure::kTau, kIris),
                TaneMiningParams(60896, 0.01, algos::AfdErrorMeasure::kTau, kIris),
                TaneMiningParams(41837, 0.1, algos::AfdErrorMeasure::kTau, kNeighbors10k),
                TaneMiningParams(57486, 0.01, algos::AfdErrorMeasure::kTau, kNeighbors10k),
                TaneMiningParams(21731, 0.01, algos::AfdErrorMeasure::kMuPlus, kTestFD),
                TaneMiningParams(60841, 0.1, algos::AfdErrorMeasure::kMuPlus, kIris),
                TaneMiningParams(60896, 0.01, algos::AfdErrorMeasure::kMuPlus, kIris),
                TaneMiningParams(43368, 0.1, algos::AfdErrorMeasure::kMuPlus, kNeighbors10k),
                TaneMiningParams(43368, 0.01, algos::AfdErrorMeasure::kMuPlus, kNeighbors10k),
                TaneMiningParams(21731, 0.01, algos::AfdErrorMeasure::kRho, kTestFD),
                TaneMiningParams(11873, 0.1, algos::AfdErrorMeasure::kRho, kIris),
                TaneMiningParams(47878, 0.01, algos::AfdErrorMeasure::kRho, kIris),
                TaneMiningParams(41837, 0.1, algos::AfdErrorMeasure::kRho, kNeighbors10k),
                TaneMiningParams(41837, 0.01, algos::AfdErrorMeasure::kRho, kNeighbors10k)));

}  // namespace tests
