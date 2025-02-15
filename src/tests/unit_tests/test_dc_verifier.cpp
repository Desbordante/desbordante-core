#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/dc/verifier/dc_verifier.h"
#include "all_csv_configs.h"
#include "config/names_and_descriptions.h"

namespace tests {

using namespace algos;
using namespace algos::dc;

namespace mo = model;

static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config, std::string dc) {
    using namespace config::names;
    return {{kCsvConfig, csv_config}, {kDenialConstraint, dc}};
}

struct DCTestParams {
    std::string dc_string;
    CSVConfig csv_config;
    bool expected;

    DCTestParams(std::string&& dc_string, CSVConfig const& csv_config, bool expected)
        : dc_string(dc_string), csv_config(csv_config), expected(expected) {}
};

class TestDCVerifier : public ::testing::TestWithParam<DCTestParams> {};

TEST_P(TestDCVerifier, DefaultTest) {
    DCTestParams const& p = GetParam();
    algos::StdParamsMap params = GetParamMap(p.csv_config, p.dc_string);
    std::unique_ptr<DCVerifier> dc_verifier = algos::CreateAndLoadAlgorithm<DCVerifier>(params);
    dc_verifier->Execute();
    bool res = dc_verifier->DCHolds();
    EXPECT_EQ(res, p.expected);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    DCVerifierTestSuite, TestDCVerifier, ::testing::Values(
    DCTestParams("!(t.Col3 == s.Col3 and s.Col1 == t.Col1 and s.Col2 == t.Col2)", kTestDC, true),
    DCTestParams("!(t.Col1 == s.Col1 and s.Col2 == t.Col2 and s.Col0 == t.Col0)", kTestDC, false),
    DCTestParams("!(s.Col0 == t.Col0 and t.Col1 == s.Col1 and s.Col2 > t.Col4)", kTestDC, true),
    DCTestParams("!(s.0 == t.0 and t.1 == s.1 and s.2 > t.4)", kTestDC, true),
    DCTestParams("!(s.0 == t.0 and t.Col1 == s.Col1 and s.Col2 > t.4)", kTestDC, true),
    DCTestParams("!(s.0 == t.1 and s.1 == t.2 and s.2 == t.3)", kBernoulliRelation, false),
    DCTestParams("!(s.Col0 == t.Col0 and s.Col5 <= t.Col6)", kTestDC, true),
    DCTestParams("!(t.Col7 > s.Col3 and s.Col1 == t.Col1)", kTestDC, true),
    DCTestParams("!(t.Col2 == s.Col2 and s.Col4 >= t.Col5)", kTestDC, true),
    DCTestParams("!(s.Salary < t.Salary and s.State == t.State and s.FedTaxRate > t.FedTaxRate)", kTestDC1, true),
    DCTestParams("!(s.Salary <= t.Salary and s.State == t.State and s.FedTaxRate >= t.FedTaxRate)", kTestDC1, false),
    DCTestParams("!(s.Salary > t.FedTaxRate and s.Salary <= t.FedTaxRate)", kTestDC1, true),
    DCTestParams("!(s.Salary == t.FedTaxRate)", kTestDC1, true),
    DCTestParams("!(s.Salary < s.Salary and t.State != t.State)", kTestDC1, true),
    DCTestParams("!(t.Salary != t.FedTaxRate and s.Salary < s.FedTaxRate and t.State == t.State)", kTestDC1, true),
    DCTestParams("!(t.Salary == t.FedTaxRate and s.Salary == s.FedTaxRate and s.Salary == t.FedTaxRate)", kTestDC1, true),
    DCTestParams("!(t.Salary == s.FedTaxRate and s.Salary == t.FedTaxRate)", kTestDC1, true),
    DCTestParams("!(t.Salary != s.FedTaxRate and s.Salary != t.FedTaxRate)", kTestDC1, false)
    )
);

// clang-format on

}  // namespace tests
