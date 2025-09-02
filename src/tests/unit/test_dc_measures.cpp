#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/dc/measures/measure.h"
#include "algorithms/dc/verifier/dc_verifier.h"
#include "all_csv_configs.h"
#include "config/names_and_descriptions.h"

namespace tests {

using namespace algos;
using namespace algos::dc;

namespace mo = model;

static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config, std::string const& dc,
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

class TestDCMeasures : public ::testing::TestWithParam<MeasureTestParams> {};

TEST_P(TestDCMeasures, DefaultTest) {
    MeasureTestParams const& p = GetParam();
    algos::StdParamsMap params = GetParamMap(p.csv_config, p.dc_string, true);
    std::shared_ptr<DCVerifier> verifier = algos::CreateAndLoadAlgorithm<DCVerifier>(params);
    verifier->Execute();
    Measure m(verifier);
    double measure_val = m.Get(p.measure_type);
    EXPECT_DOUBLE_EQ(measure_val, p.measure_val);
}

static std::string const base_dc =
        "!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate)";

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    DCMeasuresTestSuite, TestDCMeasures, ::testing::Values(
    MeasureTestParams{base_dc, kTestDC1, MeasureType::G1, 1.0},
    MeasureTestParams{base_dc, kTestDC4, MeasureType::G1, 0.958677685950413},
    MeasureTestParams{base_dc, kTestDC5, MeasureType::G1, 0.923469387755102},
    MeasureTestParams{base_dc, kTestDC1, MeasureType::G1_NORM, 1.0},
    MeasureTestParams{base_dc, kTestDC4, MeasureType::G1_NORM, 0.9545454545454545},
    MeasureTestParams{base_dc, kTestDC5, MeasureType::G1_NORM, 0.9175824175824175},
    MeasureTestParams{base_dc, kTestDC1, MeasureType::G2, 1.0},
    MeasureTestParams{base_dc, kTestDC4, MeasureType::G2, 0.942148760330578},
    MeasureTestParams{base_dc, kTestDC5, MeasureType::G2, 0.9285714285714285}
    )
);

// clang-format on

}  // namespace tests
