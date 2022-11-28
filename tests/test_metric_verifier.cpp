#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/metric_verifier.h"
#include "algorithms/metric_verifier_enums.h"
#include "algorithms/options/names.h"

namespace tests {
namespace onam = algos::config::names;

struct MetricVerifyingParams {
    algos::StdParamsMap params;
    bool const expected;

    MetricVerifyingParams(std::string const& metric,
                          long double const min_parameter,
                          std::vector<unsigned int> lhs_indices,
                          std::vector<unsigned int> rhs_indices,
                          std::string const& dataset,
                          char const separator,
                          bool const has_header,
                          std::string const& algo = "brute",
                          bool const dist_to_null_infinity = false,
                          bool const expected = true,
                          unsigned const q = 2)
        : params({{onam::kParameter,            min_parameter},
                  {onam::kLhsIndices,           std::move(lhs_indices)},
                  {onam::kRhsIndices,           std::move(rhs_indices)},
                  {onam::kData,                 std::string{std::filesystem::current_path() /
                                                            "input_data" / dataset}},
                  {onam::kSeparator,            separator},
                  {onam::kHasHeader,            has_header},
                  {onam::kEqualNulls,           true},
                  {onam::kMetric,               algos::Metric::_from_string(metric.data())},
                  {onam::kQGramLength,          q},
                  {onam::kMetricAlgorithm,      algos::MetricAlgo::_from_string(algo.data())},
                  {onam::kDistToNullIsInfinity, dist_to_null_infinity}}),
          expected(expected) {}
};

class TestMetricVerifying : public ::testing::TestWithParam<MetricVerifyingParams> {};

static std::unique_ptr<algos::MetricVerifier> CreateMetricVerifier(algos::StdParamsMap const& map) {
    auto mp = algos::StdParamsMap(map);
    return algos::CreateAndLoadPrimitive<algos::MetricVerifier>(mp);
}

static bool GetResult(algos::MetricVerifier& metric_verifier) {
    metric_verifier.Execute();
    return metric_verifier.GetResult();
}

TEST_P(TestMetricVerifying, DefaultTest) {
    auto const& params = GetParam().params;
    auto verifier = CreateMetricVerifier(params);

    if (!GetParam().expected) {
        ASSERT_FALSE(GetResult(*verifier));
        return;
    }
    ASSERT_TRUE(GetResult(*verifier));

    auto new_parameter = boost::any_cast<long double>(params.at(onam::kParameter));
    new_parameter -= 1e-4;
    if (new_parameter < 0) return;
    verifier->SetParameter(new_parameter);
    ASSERT_FALSE(GetResult(*verifier));
}

INSTANTIATE_TEST_SUITE_P(
    MetricVerifierTestSuite, TestMetricVerifying,
    ::testing::Values(
        MetricVerifyingParams("euclidean", 2, {0, 1}, {2}, "TestLong.csv", ',', true),
        MetricVerifyingParams("euclidean", 1, {0}, {1}, "TestLong.csv", ',', true),
        MetricVerifyingParams("euclidean", 4, {1}, {0}, "TestLong.csv", ',', true),
        MetricVerifyingParams("euclidean", 5, {0}, {2}, "TestLong.csv", ',', true),
        MetricVerifyingParams("euclidean", 0, {2}, {1}, "TestLong.csv", ',', true),
        MetricVerifyingParams("euclidean", 20500, {0}, {4}, "TestMetric.csv", ',', true),
        MetricVerifyingParams("euclidean", 1059, {1}, {4}, "TestMetric.csv", ',', true),
        MetricVerifyingParams("euclidean", 1, {1, 0}, {4}, "TestMetric.csv", ',', true),
        MetricVerifyingParams("euclidean", 4.5724231, {0}, {2}, "TestMetric.csv", ',', true),
        MetricVerifyingParams("euclidean", 7.53, {0}, {3}, "TestMetric.csv", ',', true),
        MetricVerifyingParams("levenshtein", 2, {0}, {5}, "TestMetric.csv", ',', true),
        MetricVerifyingParams("levenshtein", 3, {1}, {5}, "TestMetric.csv", ',', true),
        MetricVerifyingParams("levenshtein", 4, {0}, {6}, "TestMetric.csv", ',', true),
        MetricVerifyingParams(
            "levenshtein", 10, {0}, {6}, "TestMetric.csv", ',', true, "brute", true, false),
        MetricVerifyingParams(
            "cosine", 0.661938299, {0}, {7}, "TestMetric.csv", ',', true, "brute", false, true, 2),
        MetricVerifyingParams(
            "cosine", 0.5, {1}, {7}, "TestMetric.csv", ',', true, "brute", false, true, 2),
        MetricVerifyingParams(
            "cosine", 0.75, {1}, {6}, "TestMetric.csv", ',', true, "brute", false, true, 2),
        MetricVerifyingParams(
            "cosine", 0.0298575, {1}, {5}, "TestMetric.csv", ',', true, "brute", false, true, 1),
        MetricVerifyingParams(
            "cosine", 0.661938299, {0}, {8}, "TestMetric.csv", ',', true, "brute", false, true, 3),
        MetricVerifyingParams(
            "cosine", 0.525658351, {1}, {8}, "TestMetric.csv", ',', true, "brute", false, true, 3),
        MetricVerifyingParams("euclidean", 5.0990195135928, {0}, {1, 2}, "TestLong.csv", ',', true),
        MetricVerifyingParams(
            "euclidean", 5.0990195135928, {0}, {1, 2}, "TestLong.csv", ',', true, "calipers"),
        MetricVerifyingParams(
            "euclidean", 3.081374600094, {0}, {9, 10}, "TestMetric.csv", ',', true),
        MetricVerifyingParams(
            "euclidean", 3.081374600094, {0}, {9, 10}, "TestMetric.csv", ',', true, "calipers"),
        MetricVerifyingParams(
            "euclidean", 4.5, {0}, {11, 12}, "TestMetric.csv", ',', true),
        MetricVerifyingParams(
            "euclidean", 4.5, {0}, {12, 11}, "TestMetric.csv", ',', true, "calipers"),
        MetricVerifyingParams(
            "euclidean", 6.0091679956547, {0}, {13, 14, 15}, "TestMetric.csv", ',', true)
    ));

}  // namespace tests
