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

class TestTaneAfdMeasuresMining : public ::testing::TestWithParam<TaneMiningParams> {};

TEST_P(TestTaneAfdMeasuresMining, DefaultTest) {
    auto const& p = GetParam();
    algos::StdParamsMap params{{onam::kCsvConfig, p.csv_config},
                               {onam::kError, p.error},
                               {onam::kAfdErrorMeasure, p.error_measure}};
    auto tane = algos::CreateAndLoadAlgorithm<algos::Tane>(params);
    tane->Execute();
    EXPECT_EQ(p.result_hash, tane->Fletcher16());
}

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
