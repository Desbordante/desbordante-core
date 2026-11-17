#include <algorithm>
#include <memory>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/fd_verifier/fd_verifier.h"
#include "core/algorithms/fd/fd_verifier/stats_calculator.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "core/model/types/builtin.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace {
using namespace algos::fd_verifier;

void TestSorting(std::unique_ptr<FDVerifier> verifier) {
    auto const& highlights = verifier->GetHighlights();
    if (highlights.size() < 2) {
        return;
    }
    verifier->SortHighlightsByProportionDescending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsByProportionDescending()));
    verifier->SortHighlightsByProportionAscending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsByProportionAscending()));
    verifier->SortHighlightsBySizeDescending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsBySizeDescending()));
    verifier->SortHighlightsBySizeAscending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsBySizeAscending()));
    verifier->SortHighlightsByNumDescending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsByNumDescending()));
    verifier->SortHighlightsByNumAscending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               StatsCalculator::CompareHighlightsByNumAscending()));
    verifier->SortHighlightsByLhsDescending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               verifier->CompareHighlightsByLhsDescending()));
    verifier->SortHighlightsByLhsAscending();
    ASSERT_TRUE(std::is_sorted(highlights.begin(), highlights.end(),
                               verifier->CompareHighlightsByLhsAscending()));
}
}  // namespace

namespace tests {
namespace onam = config::names;

TEST(FDVerifierExecution, ExactReexecutionClearsPreviousViolations) {
    algos::StdParamsMap params{{onam::kCsvConfig, kTestFD},
                               {onam::kLhsIndices, config::IndicesType{4}},
                               {onam::kRhsIndices, config::IndicesType{3}},
                               {onam::kAfdErrorMeasure, algos::AfdErrorMeasure::kG3Prime}};
    auto verifier = algos::CreateAndLoadAlgorithm<FDVerifier>(params);
    verifier->Execute();
    ASSERT_FALSE(verifier->FDHolds());
    ASSERT_NEAR(verifier->GetError(), 1.L / 7.L, 1e-12L);
    ASSERT_EQ(verifier->GetNumErrorRows(), 2);
    ASSERT_EQ(verifier->GetHighlights().size(), 1);

    // Check an exact dependency on the same object. Previous violations must be cleared.
    verifier->SetOption(onam::kLhsIndices, config::IndicesType{3, 4});
    verifier->SetOption(onam::kRhsIndices, config::IndicesType{3});
    verifier->SetOption(onam::kAfdErrorMeasure, algos::AfdErrorMeasure::kG3Prime);
    verifier->Execute();
    EXPECT_TRUE(verifier->FDHolds());
    EXPECT_EQ(verifier->GetError(), 0.L);
    EXPECT_EQ(verifier->GetNumErrorRows(), 0);
    EXPECT_EQ(verifier->GetNumErrorClusters(), 0);
    EXPECT_TRUE(verifier->GetHighlights().empty());
}

struct ExactDependencyParams {
    char const* name;
    char const* file;
    model::ColumnIndex lhs;
    model::ColumnIndex rhs;
};

class FDVerifierExactDependencies : public ::testing::TestWithParam<ExactDependencyParams> {};

TEST_P(FDVerifierExactDependencies, ReturnsZeroError) {
    auto const& dependency = GetParam();
    algos::StdParamsMap params{
            {onam::kCsvConfig, CSVConfig{kTestDataDir / dependency.file, ',', true}},
            {onam::kLhsIndices, config::IndicesType{dependency.lhs}},
            {onam::kRhsIndices, config::IndicesType{dependency.rhs}},
            {onam::kAfdErrorMeasure, algos::AfdErrorMeasure::kFi}};
    auto verifier = algos::CreateAndLoadAlgorithm<FDVerifier>(params);
    verifier->Execute();
    EXPECT_TRUE(verifier->FDHolds());
    EXPECT_EQ(verifier->GetError(), 0.L);
}

INSTANTIATE_TEST_SUITE_P(FDVerifierExactness, FDVerifierExactDependencies,
                         ::testing::Values(ExactDependencyParams{"RepeatedLhs", "TestFD.csv", 5, 1},
                                           ExactDependencyParams{"UniqueLhs", "SimpleTypes.csv", 3,
                                                                 4},
                                           ExactDependencyParams{"ConstantRhs", "TestFD.csv", 4, 0},
                                           ExactDependencyParams{"OneRow", "OneRow.csv", 0, 1}),
                         [](auto const& info) { return info.param.name; });

struct FDVerifyingParams {
    algos::StdParamsMap params;
    long double const error = 0.;
    size_t const num_error_clusters = 0;
    size_t const num_error_rows = 0;

    FDVerifyingParams(config::IndicesType lhs_indices, config::IndicesType rhs_indices,
                      size_t const num_error_clusters = 0, size_t const num_error_rows = 0,
                      long double const error = 0., CSVConfig const& csv_config = kTestFD)
        : params({{onam::kCsvConfig, csv_config},
                  {onam::kLhsIndices, std::move(lhs_indices)},
                  {onam::kRhsIndices, std::move(rhs_indices)},
                  {onam::kEqualNulls, true}}),
          error(error),
          num_error_clusters(num_error_clusters),
          num_error_rows(num_error_rows) {}
};

class TestFDVerifying : public ::testing::TestWithParam<FDVerifyingParams> {};

TEST_P(TestFDVerifying, DefaultTest) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto verifier = algos::CreateAndLoadAlgorithm<algos::fd_verifier::FDVerifier>(mp);
    verifier->Execute();
    EXPECT_EQ(verifier->FDHolds(), p.num_error_clusters == 0);
    EXPECT_DOUBLE_EQ(verifier->GetError(), p.error);
    EXPECT_EQ(verifier->GetNumErrorRows(), p.num_error_rows);
    EXPECT_EQ(verifier->GetNumErrorClusters(), p.num_error_clusters);

    TestSorting(std::move(verifier));
}

INSTANTIATE_TEST_SUITE_P(
        FDVerifierTestSuite, TestFDVerifying,
        ::testing::Values(FDVerifyingParams({1}, {0}), FDVerifyingParams({2}, {0}),
                          FDVerifyingParams({2}, {1}), FDVerifyingParams({0, 1, 2, 3, 4}, {5}),
                          FDVerifyingParams({2, 3}, {5}), FDVerifyingParams({5}, {0}),
                          FDVerifyingParams({5}, {1}), FDVerifyingParams({5}, {2}),
                          FDVerifyingParams({5}, {3}), FDVerifyingParams({5}, {4}),
                          FDVerifyingParams({1, 3}, {4}), FDVerifyingParams({5}, {0, 1, 2, 3, 4}),
                          FDVerifyingParams({2}, {0, 1}), FDVerifyingParams({2, 3}, {0, 1, 4, 5}),
                          FDVerifyingParams({2, 4}, {0, 1, 3, 5}),
                          FDVerifyingParams({3, 4}, {0, 1}), FDVerifyingParams({1, 4}, {0, 3}),
                          FDVerifyingParams({1, 3}, {0, 3}),
                          FDVerifyingParams({4}, {3}, 1, 2, 2.L / 132),
                          FDVerifyingParams({3}, {4}, 2, 10, 26.L / 132),
                          FDVerifyingParams({0}, {1}, 1, 12, 108.L / 132),
                          FDVerifyingParams({1}, {2}, 4, 12, 16.L / 132),
                          FDVerifyingParams({1}, {3}, 2, 6, 8.L / 132),
                          FDVerifyingParams({1}, {2, 3}, 4, 12, 18.L / 132),
                          FDVerifyingParams({2}, {5}, 1, 2, 2.L / 132),
                          FDVerifyingParams({1, 3}, {5}, 3, 8, 10.L / 132),
                          FDVerifyingParams({1, 2}, {0, 3}, 1, 2, 2.L / 132),
                          FDVerifyingParams({3, 4}, {1, 2}, 3, 8, 10.L / 132),
                          FDVerifyingParams({2}, {3, 4}, 1, 2, 2.L / 132),
                          FDVerifyingParams({4}, {1, 2}, 4, 10, 12.L / 132),
                          FDVerifyingParams({0}, {2, 3}, 1, 12, 126.L / 132),
                          FDVerifyingParams({1, 4}, {2, 3, 5}, 3, 8, 10.L / 132),
                          FDVerifyingParams({0, 1}, {1, 4}, 2, 6, 8.L / 132)));
}  // namespace tests
