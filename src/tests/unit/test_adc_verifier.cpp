#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/dc/ADCVerifier/adc_verifier.h"
#include "algorithms/dc/DCVerifier/dc_verifier.h"
#include "algorithms/dc/measures/measure.h"
#include "all_csv_configs.h"
#include "config/names_and_descriptions.h"

namespace tests {

using namespace algos;
using namespace algos::dc;

namespace mo = model;

static algos::StdParamsMap GetParamsMap(CSVConfig const& csv_config, std::string const& dc,
                                        double measure_val, algos::dc::MeasureType measure_type) {
    using namespace config::names;
    return {{kCsvConfig, csv_config},
            {kDenialConstraint, dc},
            {kError, measure_val},
            {kADCErrorMeasure, measure_type}};
}

static std::string const base_dc =
        "!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate)";

struct ADCVerifierTestParams {
    std::string dc_string;
    CSVConfig csv_config;
    MeasureType measure_type;
    double error;
    bool holds;
};

class TestADCVerifier : public ::testing::TestWithParam<ADCVerifierTestParams> {};

TEST_P(TestADCVerifier, DefaultTest) {
    ADCVerifierTestParams const& p = GetParam();
    // set measure type for algorithm
    algos::StdParamsMap params = GetParamsMap(p.csv_config, p.dc_string, p.error, p.measure_type);
    std::shared_ptr<ADCVerifier> adc_verifier = algos::CreateAndLoadAlgorithm<ADCVerifier>(params);
    adc_verifier->Execute();
    EXPECT_EQ(adc_verifier->ADCHolds(), p.holds);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    ADCVerifierTestSuite, TestADCVerifier, ::testing::Values(
    ADCVerifierTestParams{base_dc, kTestDC1, MeasureType::G1, 1.0, true},
    ADCVerifierTestParams{base_dc, kTestDC4, MeasureType::G1, 0.95, true},
    ADCVerifierTestParams{base_dc, kTestDC5, MeasureType::G1, 0.93, false},
    ADCVerifierTestParams{base_dc, kTestDC1, MeasureType::G1_NORM, 1.0, true},
    ADCVerifierTestParams{base_dc, kTestDC4, MeasureType::G1_NORM, 0.96, false},
    ADCVerifierTestParams{base_dc, kTestDC5, MeasureType::G1_NORM, 0.92, false},
    ADCVerifierTestParams{base_dc, kTestDC1, MeasureType::G2, 1.0, true},
    ADCVerifierTestParams{base_dc, kTestDC4, MeasureType::G2, 0.93, true},
    ADCVerifierTestParams{base_dc, kTestDC5, MeasureType::G2, 0.9, true},
    ADCVerifierTestParams{"!(t.0 > s.3)", kLineItem, MeasureType::G1_NORM, 0.51, false}
    )
);

// clang-format on

}  // namespace tests
