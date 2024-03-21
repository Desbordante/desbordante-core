#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/metric/enums.h"
#include "algorithms/metric/metric_verifier.h"
#include "all_csv_configs.h"
#include "config/names.h"

namespace tests {
namespace onam = config::names;
using MetricAlgo = algos::metric::MetricAlgo;
using Metric = algos::metric::Metric;

struct MetricVerifyingParams {
    algos::StdParamsMap params;
    bool const expected;

    MetricVerifyingParams(CSVConfig const& csv_config, Metric metric,
                          long double const min_parameter, std::vector<unsigned int> lhs_indices,
                          std::vector<unsigned int> rhs_indices,
                          MetricAlgo algo = MetricAlgo::brute,
                          bool const dist_from_null_is_infinity = false, bool const expected = true,
                          unsigned const q = 2)
        : params({{onam::kCsvConfig, csv_config},
                  {onam::kParameter, min_parameter},
                  {onam::kLhsIndices, std::move(lhs_indices)},
                  {onam::kRhsIndices, std::move(rhs_indices)},
                  {onam::kEqualNulls, true},
                  {onam::kMetric, metric},
                  {onam::kQGramLength, q},
                  {onam::kMetricAlgorithm, algo},
                  {onam::kDistFromNullIsInfinity, dist_from_null_is_infinity}}),
          expected(expected) {}
};

struct HighlightTestParams {
    algos::StdParamsMap params;
    std::vector<std::vector<long double>> highlight_distances;

    HighlightTestParams(CSVConfig const& csv_config,
                        std::vector<std::vector<long double>>&& highlight_distances, Metric metric,
                        std::vector<unsigned int> lhs_indices,
                        std::vector<unsigned int> rhs_indices, MetricAlgo algo = MetricAlgo::brute,
                        bool const dist_from_null_is_infinity = false, unsigned const q = 2)
        : params({{onam::kCsvConfig, csv_config},
                  {onam::kParameter, (long double)0},
                  {onam::kLhsIndices, std::move(lhs_indices)},
                  {onam::kRhsIndices, std::move(rhs_indices)},
                  {onam::kEqualNulls, true},
                  {onam::kMetric, metric},
                  {onam::kQGramLength, q},
                  {onam::kMetricAlgorithm, algo},
                  {onam::kDistFromNullIsInfinity, dist_from_null_is_infinity}}),
          highlight_distances(std::move(highlight_distances)) {}
};

class TestMetricVerifying : public ::testing::TestWithParam<MetricVerifyingParams> {};

class TestHighlights : public ::testing::TestWithParam<HighlightTestParams> {};

static std::unique_ptr<algos::metric::MetricVerifier> CreateMetricVerifier(
        algos::StdParamsMap const& map) {
    auto mp = algos::StdParamsMap(map);
    return algos::CreateAndLoadAlgorithm<algos::metric::MetricVerifier>(mp);
}

static bool GetResult(algos::metric::MetricVerifier& metric_verifier) {
    metric_verifier.Execute();
    return metric_verifier.GetResult();
}

static std::vector<std::vector<algos::metric::Highlight>> GetHighlights(
        algos::metric::MetricVerifier& metric_verifier) {
    metric_verifier.Execute();
    return metric_verifier.GetHighlights();
}

TEST_P(TestMetricVerifying, DefaultTest) {
    auto const& params = GetParam().params;
    auto verifier = CreateMetricVerifier(params);

    if (!GetParam().expected) {
        ASSERT_FALSE(GetResult(*verifier));
        return;
    }
    ASSERT_TRUE(GetResult(*verifier));

    algos::ConfigureFromMap(*verifier, algos::StdParamsMap{params});
    auto new_parameter = boost::any_cast<long double>(params.at(onam::kParameter));
    new_parameter -= 1e-4;
    if (new_parameter < 0) return;
    verifier->SetParameter(new_parameter);
    ASSERT_FALSE(GetResult(*verifier));
}

TEST_P(TestMetricVerifying, ConsistentRepeatedExecution) {
    auto const& params = GetParam().params;
    auto verifier = CreateMetricVerifier(params);
    for (int i = 0; i < 5; ++i) {
        algos::ConfigureFromMap(*verifier, algos::StdParamsMap{params});
        ASSERT_EQ(GetResult(*verifier), GetParam().expected);
    }
}

TEST_P(TestHighlights, DefaultTest) {  // Assumes that highlights are sorted by distance in reverse
    auto const& params = GetParam().params;
    auto const& highlight_distances = GetParam().highlight_distances;
    auto verifier = CreateMetricVerifier(params);
    auto const& highlights = GetHighlights(*verifier);

    ASSERT_TRUE(highlights.size() == highlight_distances.size());
    for (size_t i = 0; i < highlights.size(); ++i) {
        ASSERT_TRUE(highlights[i].size() == highlight_distances[i].size());
        ASSERT_TRUE(std::equal(
                highlights[i].begin(), highlights[i].end(), highlight_distances[i].begin(),
                [](auto const& hl, long double dist) {
                    return hl.max_distance == dist || std::abs(hl.max_distance - dist) < 1e-4;
                }));
    }
}

INSTANTIATE_TEST_SUITE_P(
        MetricVerifierTestSuite, TestMetricVerifying,
        ::testing::Values(
                MetricVerifyingParams(kTestLong, Metric::euclidean, 2, {0, 1}, {2}),
                MetricVerifyingParams(kTestLong, Metric::euclidean, 1, {0}, {1}),
                MetricVerifyingParams(kTestLong, Metric::euclidean, 4, {1}, {0}),
                MetricVerifyingParams(kTestLong, Metric::euclidean, 5, {0}, {2}),
                MetricVerifyingParams(kTestLong, Metric::euclidean, 0, {2}, {1}),
                MetricVerifyingParams(kTestMetric, Metric::euclidean, 20500, {0}, {4}),
                MetricVerifyingParams(kTestMetric, Metric::euclidean, 1059, {1}, {4}),
                MetricVerifyingParams(kTestMetric, Metric::euclidean, 1, {1, 0}, {4}),
                MetricVerifyingParams(kTestMetric, Metric::euclidean, 4.5724231, {0}, {2}),
                MetricVerifyingParams(kTestMetric, Metric::euclidean, 7.53, {0}, {3}),
                MetricVerifyingParams(kTestMetric, Metric::levenshtein, 2, {0}, {5}),
                MetricVerifyingParams(kTestMetric, Metric::levenshtein, 3, {1}, {5}),
                MetricVerifyingParams(kTestMetric, Metric::levenshtein, 4, {0}, {6}),
                MetricVerifyingParams(kTestMetric, Metric::levenshtein, 10, {0}, {6},
                                      MetricAlgo::brute, true, false),
                MetricVerifyingParams(kTestMetric, Metric::cosine, 0.661938299, {0}, {7},
                                      MetricAlgo::brute, false, true, 2),
                MetricVerifyingParams(kTestMetric, Metric::cosine, 0.5, {1}, {7}, MetricAlgo::brute,
                                      false, true, 2),
                MetricVerifyingParams(kTestMetric, Metric::cosine, 0.75, {1}, {6},
                                      MetricAlgo::brute, false, true, 2),
                MetricVerifyingParams(kTestMetric, Metric::cosine, 0.0298575, {1}, {5},
                                      MetricAlgo::brute, false, true, 1),
                MetricVerifyingParams(kTestMetric, Metric::cosine, 0.661938299, {0}, {8},
                                      MetricAlgo::brute, false, true, 3),
                MetricVerifyingParams(kTestMetric, Metric::cosine, 0.525658351, {1}, {8},
                                      MetricAlgo::brute, false, true, 3),
                MetricVerifyingParams(kTestLong, Metric::euclidean, 5.0990195135928, {0}, {1, 2}),
                MetricVerifyingParams(kTestLong, Metric::euclidean, 5.0990195135928, {0}, {1, 2},
                                      MetricAlgo::calipers),
                MetricVerifyingParams(kTestMetric, Metric::euclidean, 3.081374600094, {0}, {9, 10}),
                MetricVerifyingParams(kTestMetric, Metric::euclidean, 3.081374600094, {0}, {9, 10},
                                      MetricAlgo::calipers),
                MetricVerifyingParams(kTestMetric, Metric::euclidean, 4.5, {0}, {11, 12}),
                MetricVerifyingParams(kTestMetric, Metric::euclidean, 4.5, {0}, {12, 11},
                                      MetricAlgo::calipers),
                MetricVerifyingParams(kTestMetric, Metric::euclidean, 6.0091679956547, {0},
                                      {13, 14, 15})));

constexpr long double kInf = std::numeric_limits<long double>::infinity();

INSTANTIATE_TEST_SUITE_P(
        HighlightTestSuite, TestHighlights,
        ::testing::Values(
                HighlightTestParams(kTestMetric,
                                    {{125, 125, 110, 79, 78, 68},
                                     {20500, 20500, 20000, 19997, 19991, 18900}},
                                    Metric::euclidean, {0}, {4}),
                HighlightTestParams(kTestMetric,
                                    {{4.572423, 4.572423, 4.217, 3.663899, 3.217, 3.21},
                                     {4.0331, 4.0331, 3.1101, 3.03, 2.3311, 2.3101}},
                                    Metric::euclidean, {0}, {2}),
                HighlightTestParams(kTestMetric, {{3.9, 3.9, 2.07, 0, 0}}, Metric::euclidean, {16},
                                    {13}),
                HighlightTestParams(kTestMetric, {{kInf, 3.9, 3.9, 2.07, 0}}, Metric::euclidean,
                                    {16}, {13}, MetricAlgo::brute, true),
                HighlightTestParams(kTestMetric, {{2, 2, 2, 2, 2, 1}, {2, 2, 2, 2, 2, 2}},
                                    Metric::levenshtein, {0}, {5}),
                HighlightTestParams(kTestMetric, {{4, 4, 4, 3, 3, 3}}, Metric::levenshtein, {0},
                                    {6}),
                HighlightTestParams(kTestMetric,
                                    {{0.661938, 0.661938, 0.661938, 0.6, 0.552786, 0.525658}},
                                    Metric::cosine, {0}, {7}),
                HighlightTestParams(kTestMetric, {{0.5, 0.5, 0.410744}}, Metric::cosine, {1}, {7}),
                HighlightTestParams(kTestMetric,
                                    {{0.661938, 0.661938, 0.53709, 0.525658, 0.428571, 0.316237},
                                     {kInf, kInf, kInf, 0, 0, 0}},
                                    Metric::cosine, {0}, {8}, MetricAlgo::brute, true, 3),
                HighlightTestParams(kTestMetric, {{0.525658, 0.525658, 0.483602}}, Metric::cosine,
                                    {1}, {8}, MetricAlgo::brute, true, 3),
                HighlightTestParams(kTestMetric, {{4.574986, 4.574986, 2.540827, 0, 0}},
                                    Metric::euclidean, {16}, {13, 14, 15}),
                HighlightTestParams(kTestMetric, {{kInf, 4.574986, 4.574986, 2.540827, 0}},
                                    Metric::euclidean, {16}, {13, 14, 15}, MetricAlgo::brute, true),
                HighlightTestParams(kTestMetric, {{4.543006, 4.54301, 2.41814, 0, 0}},
                                    Metric::euclidean, {16}, {13, 14}),
                HighlightTestParams(kTestMetric, {{kInf, 4.543006, 4.54301, 2.41814, 0}},
                                    Metric::euclidean, {16}, {13, 14}, MetricAlgo::brute, true),
                HighlightTestParams(kTestMetric, {{4.543006, 4.54301, 2.41814, 0, 0}},
                                    Metric::euclidean, {16}, {13, 14}, MetricAlgo::calipers),
                HighlightTestParams(kTestMetric, {{kInf, 4.543006, 4.54301, 2.41814, 0}},
                                    Metric::euclidean, {16}, {13, 14}, MetricAlgo::calipers, true),
                HighlightTestParams(kTestMetric,
                                    {{3.08137, 3.08137, 2.43892, 2.15623, 2.15421, 1.86557}},
                                    Metric::euclidean, {0}, {9, 10}),
                HighlightTestParams(kTestMetric,
                                    {{3.08137, 3.08137, 2.43892, 2.15623, 2.15421, 1.86557}},
                                    Metric::euclidean, {0}, {9, 10}, MetricAlgo::calipers)));

}  // namespace tests
