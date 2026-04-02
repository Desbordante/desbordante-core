#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/dc/adc_verifier/adc_verifier.h"
#include "core/algorithms/dc/dc_verifier/dc_verifier.h"
#include "core/algorithms/dc/measures/measure.h"
#include "core/config/names_and_descriptions.h"
#include "tests/common/all_csv_configs.h"

namespace tests {

using namespace algos;
using namespace algos::dc;

namespace mo = model;

static algos::StdParamsMap GetParamsMap(CSVConfig const& csv_config, std::string const& dc,
                                        bool do_collect_violations) {
    using namespace config::names;
    return {{kCsvConfig, csv_config},
            {kDenialConstraint, dc},
            {kDoCollectViolations, do_collect_violations}};
}

struct MeasureTestParams {
    std::string dc_string;
    CSVConfig csv_config;
    MeasureType measure_type;
    double measure_val;
};

class TestADCMeasures : public ::testing::TestWithParam<MeasureTestParams> {};

TEST_P(TestADCMeasures, DefaultTest) {
    MeasureTestParams const& p = GetParam();
    algos::StdParamsMap params = GetParamsMap(p.csv_config, p.dc_string, true);
    std::shared_ptr<DCVerifier> verifier = algos::CreateAndLoadAlgorithm<DCVerifier>(params);
    verifier->Execute();
    Measure m(verifier);
    double measure_val = m.Get(p.measure_type);
    EXPECT_DOUBLE_EQ(measure_val, p.measure_val);
}

static std::string const kBaseDc =
        "!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate)";

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    ADCMeasuresTestSuite, TestADCMeasures, ::testing::Values(
    MeasureTestParams{kBaseDc, kTestDC1, MeasureType::G1, 1.0},
    MeasureTestParams{kBaseDc, kTestDC4, MeasureType::G1, 0.958677685950413},
    MeasureTestParams{kBaseDc, kTestDC5, MeasureType::G1, 0.923469387755102},
    MeasureTestParams{kBaseDc, kTestDC1, MeasureType::G1Norm, 1.0},
    MeasureTestParams{kBaseDc, kTestDC4, MeasureType::G1Norm, 0.9545454545454545},
    MeasureTestParams{kBaseDc, kTestDC5, MeasureType::G1Norm, 0.9175824175824175},
    MeasureTestParams{kBaseDc, kTestDC1, MeasureType::G2, 1.0},
    MeasureTestParams{kBaseDc, kTestDC4, MeasureType::G2, 0.942148760330578},
    MeasureTestParams{kBaseDc, kTestDC5, MeasureType::G2, 0.9285714285714285}
    )
);
// clang-format on

}  // namespace tests
