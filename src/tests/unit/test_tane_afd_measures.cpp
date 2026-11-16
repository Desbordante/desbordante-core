#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/tane/tane.h"
#include "core/config/names.h"
#include "core/util/enum_to_str.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {
namespace onam = config::names;

namespace {

constexpr config::ErrorType kTaneTolerance = 1e-10;

std::unique_ptr<algos::Tane> RunTane(algos::AfdErrorMeasure measure, config::ErrorType max_error,
                                     config::MaxLhsType max_lhs, CSVConfig const& csv_config) {
    algos::StdParamsMap params{{onam::kCsvConfig, csv_config},
                               {onam::kError, max_error},
                               {onam::kAfdErrorMeasure, measure},
                               {onam::kMaximumLhs, max_lhs}};
    auto tane = algos::CreateAndLoadAlgorithm<algos::Tane>(params);
    tane->Execute();
    return tane;
}

::AFD const* FindAfd(algos::Tane const& tane, std::vector<model::ColumnIndex> const& lhs,
                     model::ColumnIndex rhs) {
    for (::AFD const& afd : tane.AfdList()) {
        if (afd.GetLhsIndices() == lhs && afd.GetRhsIndex() == rhs) return &afd;
    }
    return nullptr;
}

}  // namespace

// ---- Legacy mining-result checks ----

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

// ---- Empty-LHS and one-row inputs ----

struct TaneEdgeCaseParams {
    algos::AfdErrorMeasure measure;
    config::ErrorType empty_lhs_error;
    config::ErrorType one_row_error;
};

class TestTaneAfdEdgeCases : public ::testing::TestWithParam<TaneEdgeCaseParams> {};

TEST_P(TestTaneAfdEdgeCases, EmptyLhsError) {
    auto const& p = GetParam();
    auto tane = RunTane(p.measure, 1.0, 0, kTestFD);

    ::AFD const* afd = FindAfd(*tane, {}, 3);
    ASSERT_NE(afd, nullptr);
    EXPECT_NEAR(afd->GetThreshold(), p.empty_lhs_error, kTaneTolerance);

    // Column 0 is constant. Every measure must give its empty-LHS dependency zero error.
    ::AFD const* constant_afd = FindAfd(*tane, {}, 0);
    ASSERT_NE(constant_afd, nullptr);
    EXPECT_EQ(constant_afd->GetThreshold(), 0);
}

TEST_P(TestTaneAfdEdgeCases, OneRowDependencies) {
    auto const& p = GetParam();
    CSVConfig const one_row{kTestDataDir / "OneRow.csv", ',', true};
    for (config::MaxLhsType max_lhs : {0, 1}) {
        SCOPED_TRACE(::testing::Message() << "maximum LHS size=" << max_lhs);
        auto tane = RunTane(p.measure, 1.0, max_lhs, one_row);
        if (std::isinf(p.one_row_error)) {
            // G1 has no row pairs to compare. Neither empty nor nonempty LHSs pass.
            EXPECT_TRUE(tane->AfdList().empty());
            continue;
        }

        // Both columns are constant. TANE reports the two empty-LHS dependencies
        // and skips larger LHSs because those dependencies would not be minimal.
        EXPECT_EQ(tane->AfdList().size(), 2);
        for (model::ColumnIndex rhs : {0, 1}) {
            SCOPED_TRACE(::testing::Message() << "RHS column=" << rhs);
            ::AFD const* afd = FindAfd(*tane, {}, rhs);
            ASSERT_NE(afd, nullptr);
            EXPECT_EQ(afd->GetThreshold(), p.one_row_error);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
        AllMeasures, TestTaneAfdEdgeCases,
        ::testing::Values(TaneEdgeCaseParams{algos::AfdErrorMeasure::kG1, 45.0 / 66.0,
                                             std::numeric_limits<config::ErrorType>::infinity()},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kPdep, 5.0 / 8.0, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kTau, 1, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kMuPlus, 1, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kRho, 3.0 / 4.0, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kFi, 1, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kG2, 1, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kG3, 1.0 / 2.0, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kG1S, 1, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kRfiPlus, 1, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kRfiPrimePlus, 1, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kSfi, 1, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kG1Prime, 45.0 / 63.0, 0},
                          TaneEdgeCaseParams{algos::AfdErrorMeasure::kG3Prime, 6.0 / 11.0, 0}),
        [](auto const& info) { return std::string{util::EnumToStr(info.param.measure)}; });

}  // namespace tests
