#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "core/algorithms/fd/tane/tane.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {
using namespace config::names;
using namespace algos::afd_metric_calculator;

struct G1PrimeAgreementParams {
    char const* file;
    model::ColumnIndex constant_column;
    bool zeroary;
    double expected_error;
};

class TestAFDMetricTaneAgreement : public ::testing::TestWithParam<G1PrimeAgreementParams> {};

TEST_P(TestAFDMetricTaneAgreement, G1PrimeStandaloneAndTaneAgreeWithMinimalityPruning) {
    auto const& p = GetParam();
    CSVConfig const csv{kTestDataDir / p.file, ',', true};
    config::IndicesType const lhs = p.zeroary ? config::IndicesType{} : config::IndicesType{0};
    // Ordinary threshold is below the zero-ary error, so TANE retains this
    // candidate instead of pruning its RHS.
    algos::StdParamsMap tane_params{{kCsvConfig, csv},
                                    {kError, p.expected_error + 1e-10},
                                    {kAfdErrorMeasure, algos::AfdErrorMeasure::kG1Prime},
                                    {kMaximumLhs, config::MaxLhsType{1}}};
    auto tane = algos::CreateAndLoadAlgorithm<algos::Tane>(tane_params);
    tane->Execute();

    // Standalone rejects an empty LHS. The constant column represents the same
    // partition and does not change complete-row duplicate counts.
    algos::StdParamsMap calculator_params{
            {kCsvConfig, csv},
            {kLhsIndices, p.zeroary ? config::IndicesType{p.constant_column} : lhs},
            {kRhsIndices, config::IndicesType{1}},
            {kMetric, AFDMetric::kG1Prime}};
    auto calculator = algos::CreateAndLoadAlgorithm<AFDMetricCalculator>(calculator_params);
    calculator->Execute();

    bool found = false;
    for (::AFD const& afd : tane->AfdList()) {
        if (afd.GetLhsIndices() != lhs || afd.GetRhsIndex() != 1) continue;
        found = true;
        EXPECT_NEAR(afd.GetThreshold(), p.expected_error, 1e-12L);
        EXPECT_NEAR(1.L - calculator->GetResult(), afd.GetThreshold(), 1e-12L);
    }
    EXPECT_TRUE(found);
}

// Six pairs, one full duplicate: denominator 5; two ordinary violations and
// four zero-ary violations. Splitting the duplicate changes the denominator to 6.
INSTANTIATE_TEST_SUITE_P(
        G1PrimeAgreementSuite, TestAFDMetricTaneAgreement,
        ::testing::Values(G1PrimeAgreementParams{"G1PrimeMiningDuplicates.csv", 2, false, 2.0 / 5},
                          G1PrimeAgreementParams{"G1PrimeMiningDuplicates.csv", 2, true, 4.0 / 5},
                          G1PrimeAgreementParams{"G1PrimeMiningProjection.csv", 3, false, 2.0 / 6},
                          G1PrimeAgreementParams{"G1PrimeMiningProjection.csv", 3, true, 4.0 / 6}));

}  // namespace tests
