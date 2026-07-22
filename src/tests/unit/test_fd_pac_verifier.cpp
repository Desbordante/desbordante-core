#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/fd_pac_verifier.h"
#include "core/config/custom_metric/custom_metrics/type.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "core/util/custom_metric/custom_metric.h"
#include "gtest/gtest.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
using namespace config::names;
using namespace pac::model;

constexpr static auto kThreshold = 1e-3;

struct FDPACVerifyingParams {
    std::size_t rhs_arity;
    algos::StdParamsMap params;
    double exp_epsilon;
    double exp_delta;

    FDPACVerifyingParams(CSVConfig const& csv_config, config::IndicesType&& lhs_indices,
                         config::IndicesType&& rhs_indices, double expected_epsilon,
                         double expected_delta, std::vector<double> lhs_deltas = {},
                         double min_epsilon = -1, double max_epsilon = -1, double min_delta = -1,
                         unsigned long delta_steps = 0,
                         config::CustomMetricsType&& lhs_metrics = {},
                         config::CustomMetricsType&& rhs_metrics = {},
                         double diagonal_threshold = 1e-5)
        : rhs_arity(rhs_indices.size()),
          params({{kCsvConfig, csv_config},
                  {kLhsIndices, std::move(lhs_indices)},
                  {kRhsIndices, std::move(rhs_indices)},
                  {kLhsDeltas, std::move(lhs_deltas)},
                  {kMinDelta, min_delta},
                  {kMinEpsilon, min_epsilon},
                  {kMaxEpsilon, max_epsilon},
                  {kDeltaSteps, delta_steps},
                  {kLhsMetrics, std::move(lhs_metrics)},
                  {kRhsMetrics, std::move(rhs_metrics)},
                  {kDiagonalThreshold, diagonal_threshold}}),
          exp_epsilon(expected_epsilon),
          exp_delta(expected_delta) {}
};

class TestFDPACVerifier : public testing::TestWithParam<FDPACVerifyingParams> {};

TEST_P(TestFDPACVerifier, DefaultTest) {
    auto const& p = GetParam();
    auto verifier = algos::CreateAndLoadAlgorithm<algos::pac_verifier::FDPACVerifier>(p.params);
    verifier->Execute();

    auto const& pac = verifier->GetPAC();
    auto const& epsilons = pac.GetEpsilons();
    ASSERT_EQ(epsilons.size(), p.rhs_arity);
    for (std::size_t i = 0; i < p.rhs_arity; ++i) {
        EXPECT_NEAR(epsilons[i], p.exp_epsilon, kThreshold);
    }
    EXPECT_NEAR(pac.GetDelta(), p.exp_delta, kThreshold);
}

auto const kAlphabetMetric = [](std::optional<std::string> const& a,
                                std::optional<std::string> const& b) {
    if (!a || !b) {
        return 0;
    }
    return std::abs(a->front() - b->front());
};

auto const kStaticStringMetric =
        std::make_shared<util::StaticCustomMetric<std::string>>(kAlphabetMetric);
auto const kDynamicStringMetric = std::make_shared<util::DynamicCustomMetric>(
        [](model::Type const* type, std::byte const* first, std::byte const* second) {
            auto first_str = type->ValueToString(first);
            auto second_str = type->ValueToString(second);
            return kAlphabetMetric(first_str, second_str);
        });

INSTANTIATE_TEST_SUITE_P(
        FDPACVerifierTests, TestFDPACVerifier,
        testing::ValuesIn({
                // -- Refinement --
                // Quite ordinary FD PAC
                // #0
                FDPACVerifyingParams(kMetricCoords, {2}, {3}, 0.027, 0.962, {0.2}),
                // String data test
                FDPACVerifyingParams(kTestND, {1}, {2}, 1, 0.889, {0.2}, -1, -1, 0.7),
                // Multi-column values
                FDPACVerifyingParams(kTestFDPAC, {0, 1}, {2, 3}, 0.07, 0.890, {1, 10}, -1, -1, 0.7),
                // PAC only on the right-hand side
                FDPACVerifyingParams(kMetricMovies, {1}, {2}, 6, 0.933, {1}, -1, -1, 0.8),
                // Pure FD
                FDPACVerifyingParams(kMetricMovies, {0, 1}, {2}, 0, 1),
                // Single cluster on the left-hand side ("anti-UCC PAC")
                FDPACVerifyingParams(kMetricCoords, {2}, {3}, 120.093, 0.787, {20}, -1, -1, 0.7),
                // Check that min_pairs = 0 does not spoil the result
                FDPACVerifyingParams(kMetricCoords, {2}, {3}, 120.093, 0.787, {20}, -1, -1, 0),
                // Different combinations of custom and default metrics
                //   a. Explicitly say that we want to use default metrics
                FDPACVerifyingParams(kMarineUrchins, {0}, {1}, 22, 0.993, {10}, -1, -1, 0.7, 400,
                                     {nullptr}, {nullptr}),
                //   b. [default] -> [custom]
                FDPACVerifyingParams(kMarineUrchins, {0}, {2}, 2, 0.686, {10}, -1, -1, 0.6, 0, {},
                                     {kStaticStringMetric}),
                //   b. [default, custom] -> [default]
                FDPACVerifyingParams(kMarineUrchins, {0, 2}, {1}, 13, 0.98, {10, 1}, -1, -1, 0.9,
                                     350, {nullptr, kDynamicStringMetric}, {}),
                // -- Parametrized refinement --
                // #10
                // Both min epsilon and max epsilon
                FDPACVerifyingParams(kMetricCoords, {2}, {3}, 0.007, 0.66, {0.2}, 0.005, 0.025),
                // Only max epsilon
                FDPACVerifyingParams(kMetricCoords, {2}, {3}, 0.007, 0.66, {0.2}, -1, 0.025),
                // Only min epsilon
                FDPACVerifyingParams(kMetricMovies, {1}, {2}, 6, 0.933, {0.2}, 5.5, -1),
                // Only min delta
                FDPACVerifyingParams(kMetricMovies, {1}, {2}, 6, 0.933, {0.2}, -1, -1, 0.7),
                // Min epsilon and min delta
                FDPACVerifyingParams(kMetricMovies, {1}, {2}, 6, 0.933, {0.2}, 4.9, -1, 0.73),
                // Max epsilon and min delta
                FDPACVerifyingParams(kMetricMovies, {1}, {2}, 1, 0.6, {0.2}, -1, 1.9, 0.5),
                // All bounds
                FDPACVerifyingParams(kMetricMovies, {1}, {2}, 5, 0.733, {0.2}, 0.9, 6.5, 0.7),
                // Validation
                // #17
                //   a. Find epsilon by delta
                FDPACVerifyingParams(kMetricCoords, {2}, {3}, 0.027, 0.962, {0.2}, 0, 0, 0.962),
                //   b. Find delta by epsilon
                FDPACVerifyingParams(kMetricCoords, {2}, {3}, 0.027, 0.962, {0.2}, 0.027, 0.027),
        }));

INSTANTIATE_TEST_SUITE_P(
        FDPACVerifierHeavyDatasets, TestFDPACVerifier,
        testing::Values(
                // Custom metrics
                //  a. Default diagonal threshold
                //     Also check that two pointers to a single metrics don't interfere
                FDPACVerifyingParams(kMushroom, {0}, {3}, 18, 0.936, {0}, 0.5, -1, -1, 0,
                                     {kStaticStringMetric}, {kStaticStringMetric}),
                //  b. Bigger diagonal threshold
                FDPACVerifyingParams(kMushroom, {0}, {3}, 11, 0.759, {0}, 0.5, -1, -1, 0,
                                     {kDynamicStringMetric}, {kDynamicStringMetric}, 0.015)));

using IndexPairs = std::vector<std::pair<std::size_t, std::size_t>>;

struct FDPACHighlightParams {
    algos::StdParamsMap params;
    double eps_1;
    double eps_2;
    IndexPairs expected_highlight;

    FDPACHighlightParams(CSVConfig const& csv_config, config::IndicesType&& lhs_indices,
                         config::IndicesType&& rhs_indices, IndexPairs&& expected_highlight,
                         double eps_1 = 0, double eps_2 = -1, std::vector<double> lhs_deltas = {1},
                         double min_delta = -1, unsigned long delta_steps = 0)
        : params({
                  {kCsvConfig, csv_config},
                  {kLhsIndices, std::move(lhs_indices)},
                  {kRhsIndices, std::move(rhs_indices)},
                  {kLhsDeltas, std::move(lhs_deltas)},
                  {kMinDelta, min_delta},
                  {kDeltaSteps, delta_steps},
          }),
          eps_1(eps_1),
          eps_2(eps_2),
          expected_highlight(std::move(expected_highlight)) {}
};

class TestFDPACHighlight : public testing::TestWithParam<FDPACHighlightParams> {};

TEST_P(TestFDPACHighlight, DefaultTest) {
    auto const& p = GetParam();
    auto verifier = algos::CreateAndLoadAlgorithm<algos::pac_verifier::FDPACVerifier>(p.params);
    verifier->Execute();

    auto highlight = verifier->GetHighlights(p.eps_1, p.eps_2);
    auto highlight_indices = highlight.RowIndices();
    EXPECT_THAT(highlight_indices, testing::UnorderedElementsAreArray(p.expected_highlight));
}

INSTANTIATE_TEST_SUITE_P(
        FDPACHighlightsTests, TestFDPACHighlight,
        testing::Values(
                // LHS values are "pure FD", so highlights are the same as would be in FD verifier
                FDPACHighlightParams(kMetricAddresses, {1}, {2},
                                     {{1, 2},
                                      {2, 1},
                                      {4, 5},
                                      {5, 4},
                                      {4, 6},
                                      {6, 4},
                                      {7, 8},
                                      {8, 7},
                                      {10, 11},
                                      {11, 10}},
                                     0, 5),
                // FDPACVerifierTests/3
                FDPACHighlightParams(
                        kTestFDPAC, {0, 1}, {2, 3},
                        {{6, 7}, {7, 6}, {6, 8}, {8, 6}, {15, 16}, {16, 15}, {16, 17}, {17, 16}},
                        0.8, 3, {1, 10})));
}  // namespace tests
