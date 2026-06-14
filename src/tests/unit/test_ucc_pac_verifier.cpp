#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/pac/pac_verifier/ucc_pac_verifier/ucc_pac_verifier.h"
#include "core/config/custom_metric/custom_vector_metric.h"
#include "core/config/exceptions.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
#include "core/model/types/builtin.h"
#include "core/model/types/type.h"
#include "gtest/gtest.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
using namespace config::names;
using namespace pac::model;

constexpr static auto kThreshold = 1e-3;

struct UCCPACVerifyingParams {
    algos::StdParamsMap params;
    double exp_epsilon;
    double exp_delta;

    UCCPACVerifyingParams(CSVConfig const& csv_config, config::IndicesType&& column_indices,
                          double expected_epsilon, double expected_delta, double max_delta = -1,
                          double min_epsilon = -1, double max_epsilon = -1,
                          unsigned long delta_steps = 0,
                          std::shared_ptr<config::ICustomVectorMetric>&& metric = nullptr,
                          double diagonal_threshold = 1e-5)
        : params({
                  {kCsvConfig, csv_config},
                  {kColumnIndices, std::move(column_indices)},
                  {kMaxDelta, max_delta},
                  {kMinEpsilon, min_epsilon},
                  {kMaxEpsilon, max_epsilon},
                  {kDeltaSteps, delta_steps},
                  {kMetric, std::move(metric)},
                  {kDiagonalThreshold, diagonal_threshold},
          }),
          exp_epsilon(expected_epsilon),
          exp_delta(expected_delta) {}
};

class TestUCCPACVerifier : public testing::TestWithParam<UCCPACVerifyingParams> {};

TEST_P(TestUCCPACVerifier, DefaultTest) {
    auto const& p = GetParam();
    auto verifier = algos::CreateAndLoadAlgorithm<algos::pac_verifier::UCCPACVerifier>(p.params);
    verifier->Execute();

    auto const& pac = verifier->GetPAC();
    EXPECT_NEAR(pac.GetEpsilon(), p.exp_epsilon, kThreshold);
    EXPECT_NEAR(pac.GetDelta(), p.exp_delta, kThreshold);
}

auto const kAlphabetMetric = [](std::string const& a, std::string const& b) {
    return std::abs(a.front() - b.front());
};

auto const kDynamicStringMetric = std::make_shared<config::CustomVectorMetric>(
        [](std::vector<model::Type const*> const& types, std::vector<std::byte const*> const& first,
           std::vector<std::byte const*> const& second) {
            std::size_t result = 0;
            for (std::size_t i = 0; i < types.size(); ++i) {
                auto const* type = types[i];
                auto first_str = type->ValueToString(first[i]);
                auto second_str = type->ValueToString(second[i]);
                result += kAlphabetMetric(first_str, second_str);
            }
            return result;
        });

INSTANTIATE_TEST_SUITE_P(
        UCCPACVerifierTests, TestUCCPACVerifier,
        testing::ValuesIn({
                // Quite ordinary UCC PAC
                //  a. Unlike other PAC verifiers, UCC PAC verifier tries to minimize delta, so we
                //     can get delta=0, even though there are pairs on the diagonal
                UCCPACVerifyingParams(kMetricMovies, {2}, 0, 0),
                //  b. Again, check that delta is being minimized correctly
                UCCPACVerifyingParams(kMetricMovies, {2}, 11, 0.402, 0.7, -1, -1, 1000),
                // Another quite ordinary UCC PAC
                UCCPACVerifyingParams(kTestFDPAC, {0}, 0.23, 0.11),
                // Custom metric
                UCCPACVerifyingParams(
                        kMarineUrchins, {1, 2}, 1, 0.173, 0.25, -1, -1, 0,
                        std::make_shared<config::CustomVectorMetric>(
                                [](auto const&, auto const& first, auto const& second) {
                                    assert(first.size() == 2 && second.size() == 2);
                                    double int_dist =
                                            std::abs(model::Type::GetValue<model::Int>(first[0]) -
                                                     model::Type::GetValue<model::Int>(second[0]));
                                    auto char_dist = std::abs(
                                            model::Type::GetValue<model::String>(first[1])[0] -
                                            model::Type::GetValue<model::String>(second[1])[0]);
                                    if (char_dist == 0) {
                                        return int_dist / 10;
                                    }
                                    return int_dist / char_dist;
                                })),
                // "Verification"
                // FIXME: This test fails!
                UCCPACVerifyingParams(kMetricMovies, {2}, 11, 0.402, 0.402, -1, -1),
        }));

TEST(UCCPACVerifierTests, DefaultMetricFails) {
    // Check that an attempt to use default metric on non-metrizable column results in clear error
    algos::StdParamsMap params{
            {kCsvConfig, kMixedWithNulls},
            {kColumnIndices, config::IndicesType{0}},
    };
    auto verifier = algos::CreateAndLoadAlgorithm<algos::pac_verifier::UCCPACVerifier>(params);
    EXPECT_THROW(verifier->Execute(), config::ConfigurationError);
}

using IndexPairs = std::vector<std::pair<std::size_t, std::size_t>>;

struct UCCPACHighlightParams {
    algos::StdParamsMap params;
    double eps_1;
    double eps_2;
    IndexPairs expected_highlight;

    UCCPACHighlightParams(CSVConfig const& csv_config, config::IndicesType&& column_indices,
                          IndexPairs&& expected_highlight, double eps_1 = 0, double eps_2 = -1,
                          double max_delta = -1, unsigned long delta_steps = 0)
        : params({
                  {kCsvConfig, csv_config},
                  {kColumnIndices, column_indices},
                  {kMaxDelta, max_delta},
                  {kDeltaSteps, delta_steps},
          }),
          eps_1(eps_1),
          eps_2(eps_2),
          expected_highlight(std::move(expected_highlight)) {}
};

class TestUCCPACHighlight : public testing::TestWithParam<UCCPACHighlightParams> {};

TEST_P(TestUCCPACHighlight, DefaultTest) {
    auto const& p = GetParam();
    auto verifier = algos::CreateAndLoadAlgorithm<algos::pac_verifier::UCCPACVerifier>(p.params);
    verifier->Execute();

    auto highlight = verifier->GetHighlights(p.eps_1, p.eps_2);
    auto highlight_indices = highlight.RowIndices();
    EXPECT_THAT(highlight_indices, testing::UnorderedElementsAreArray(p.expected_highlight));
}

INSTANTIATE_TEST_SUITE_P(
        UCCPACHighlightTests, TestUCCPACHighlight,
        testing::ValuesIn({
                UCCPACHighlightParams(kMetricMovies, {2},
                                      {
                                              // FIXME: This test fails!
                                              // Highlights are (xx, xx], so (134, 144)
                                              // must be here, while (135, 140) -- not
                                              {2, 5},  {5, 2},  {2, 6},  {6, 2},  {3, 6}, {6, 3},
                                              {3, 7},  {7, 3},  {4, 7},  {7, 4},  {4, 8}, {8, 4},
                                              {5, 7},  {7, 5},  {5, 8},  {8, 5},  {8, 9}, {9, 8},
                                              {9, 10}, {10, 9}, {9, 11}, {11, 9},
                                      },
                                      5, 10),
        }));
// TODO: moar highlight tests!
}  // namespace tests
