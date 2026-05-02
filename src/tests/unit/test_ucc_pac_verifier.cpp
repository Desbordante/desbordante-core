#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/pac/pac_verifier/ucc_pac_verifier/ucc_pac_verifier.h"
#include "core/config/custom_metric/custom_vector_metric.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"
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
        testing::Values(
                // Quite ordinary UCC PAC
                //  a. Pairs on the diagonal are considered as well, so we'll never get delta = 0
                UCCPACVerifyingParams(kMetricMovies, {2}, 0, 0.083),
                //  b. This dataset does not suit well for UCC PACs, so we get quite ugly (eps,
                //  delta)
                UCCPACVerifyingParams(kMetricMovies, {2}, 11, 0.43, 0.7)));
}  // namespace tests
