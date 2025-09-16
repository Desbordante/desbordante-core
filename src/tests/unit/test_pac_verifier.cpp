#include <string>
#include <iostream>
#include <vector>

#include <gtest/gtest.h>

#include "algo_factory.h"
#include "algorithms/pac/pac_verifier/domain_pac_verifier.h"
#include "all_csv_configs.h"
#include "csv_parser/csv_parser.h"
#include "gtest/gtest.h"
#include "indices/type.h"
#include "names.h"
#include "pac/domain_pac.h"
#include "pac/model/metrizable_tuple.h"

namespace tests {
using namespace config::names;

// Threshold for algo results (epsilon and delta)
constexpr static auto kThreshold = 1e-8;

struct DomainPACVerifyingParams {
    algos::StdParamsMap params;
    double exp_epsilon;
    double exp_delta;

    DomainPACVerifyingParams(CSVConfig const& csv_config, config::IndicesType&& col_indices,
                             std::vector<std::string> first_str, std::vector<std::string> last_str,
                             double expected_epsilon, double expected_delta, double min_epsilon = 0,
                             double max_epsilon = 1, unsigned long epsilon_steps = 100,
                             double min_delta = 0.9, double diagonal_threshold = 1e-5,
                             pac::model::Metric metric = nullptr,
                             pac::model::Comparer comparer = nullptr)
        : params({{kCsvConfig, csv_config},
                  {kColumnIndices, std::move(col_indices)},
                  {kFirst, first_str},
                  {kLast, last_str},
                  {kMinEpsilon, min_epsilon},
                  {kMaxEpsilon, max_epsilon},
                  {kEpsilonSteps, epsilon_steps},
                  {kMinDelta, min_delta},
                  {kDiagonalThreshold, diagonal_threshold},
                  {kMetric, metric},
                  {kComparer, comparer}}),
          exp_epsilon(expected_epsilon),
          exp_delta(expected_delta) {
        std::cout << "Diagonal threshold in test initialization: " << diagonal_threshold << '\n';
    }
};

class TestDomainPACVerifier : public ::testing::TestWithParam<DomainPACVerifyingParams> {};

TEST_P(TestDomainPACVerifier, DefaultTest) {
    auto const& p = GetParam();
    auto params_map = p.params;
    auto verifier =
            algos::CreateAndLoadAlgorithm<algos::pac_verifier::DomainPACVerifier>(params_map);
    verifier->Execute();

    auto pac = verifier->GetPAC();
    auto const* domain_pac = dynamic_cast<model::DomainPAC const*>(pac.get());
    EXPECT_TRUE(domain_pac);
    EXPECT_NEAR(pac->GetEpsilon(), p.exp_epsilon, kThreshold);
    EXPECT_NEAR(pac->GetDelta(), p.exp_delta, kThreshold);
}

INSTANTIATE_TEST_SUITE_P(DomainPACVerifierTests, TestDomainPACVerifier,
                         // This test is likely to be temporary
                         ::testing::Values(DomainPACVerifyingParams(kSimpleTypos, {1}, {"0"}, {"5"},
                                                                    0, 0.9, 0, 10, 100, 0.8, 1e-5)));
}  // namespace tests
