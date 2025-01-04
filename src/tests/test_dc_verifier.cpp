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
    bool dc_holds;
    std::vector<std::pair<size_t, size_t>> violations;
};

class TestDCVerifier : public ::testing::TestWithParam<DCTestParams> {};

TEST_P(TestDCVerifier, DefaultTest) {
    DCTestParams const& p = GetParam();
    algos::StdParamsMap params = GetParamMap(p.csv_config, p.dc_string);
    std::unique_ptr<DCVerifier> verifier = algos::CreateAndLoadAlgorithm<DCVerifier>(params);
    verifier->Execute();
    bool dc_holds = verifier->DCHolds();
    std::vector<std::pair<size_t, size_t>> violations = verifier->GetViolations();
    EXPECT_EQ(violations, p.violations);
    EXPECT_EQ(dc_holds, p.dc_holds);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    DCVerifierTestSuite, TestDCVerifier, ::testing::Values(
    DCTestParams("!(t.Col3 == s.Col3 and s.Col1 == t.Col1 and s.Col2 == t.Col2)", kTestDC, true, {}),
    DCTestParams("!(t.Col1 == s.Col1 and s.Col2 == t.Col2 and s.Col0 == t.Col0)", kTestDC, false, {{5, 2}}),
    DCTestParams("!(s.Col0 == t.Col0 and t.Col1 == s.Col1 and s.Col2 > t.Col4)", kTestDC, true, {}),
    DCTestParams("!(s.0 == t.0 and t.1 == s.1 and s.2 > t.4)", kTestDC, true, {}),
    DCTestParams("!(s.0 == t.0 and t.Col1 == s.Col1 and s.Col2 > t.4)", kTestDC, true, {}),
    DCTestParams("!(s.0 == t.1 and s.1 == t.2 and s.2 == t.3)", kBernoulliRelation, false, {{3, 4}, {4, 5}, {3, 7}}),
    DCTestParams("!(s.Col0 == t.Col0 and s.Col5 <= t.Col6)", kTestDC, true, {}),
    DCTestParams("!(t.Col7 > s.Col3 and s.Col1 == t.Col1)", kTestDC, true, {}),
    DCTestParams("!(t.Col2 == s.Col2 and s.Col4 >= t.Col5)", kTestDC, true, {}),
    DCTestParams("!(s.Salary < t.Salary and s.State == t.State and s.FedTaxRate > t.FedTaxRate)", kTestDC1, true, {}),
    DCTestParams("!(s.Salary <= t.Salary and s.State == t.State and s.FedTaxRate >= t.FedTaxRate)", kTestDC1, false, {{10, 11}}),
    DCTestParams("!(s.Salary > t.FedTaxRate and s.Salary <= t.FedTaxRate)", kTestDC1, true, {}),
    DCTestParams("!(s.Salary == t.FedTaxRate)", kTestDC1, true, {}),
    DCTestParams("!(s.Salary < s.Salary and t.State != t.State)", kTestDC1, true, {}),
    DCTestParams("!(t.Salary != t.FedTaxRate and s.Salary < s.FedTaxRate and t.State == t.State)", kTestDC1, true, {}),
    DCTestParams("!(t.Salary == t.FedTaxRate and s.Salary == s.FedTaxRate and s.Salary == t.FedTaxRate)", kTestDC1, true, {}),
    DCTestParams("!(t.Salary == s.FedTaxRate and s.Salary == t.FedTaxRate)", kTestDC1, true, {}),
    DCTestParams("!(t.Salary == s.FedTaxRate and s.Salary != t.FedTaxRate)", kTestDC1, true, {}),
    DCTestParams("!(s.1 == s.2)", kTestDC3, false, {{4, 4}, {7, 7}, {14, 14}}),
    DCTestParams("!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate)", kTestDC2, false,
                 {{5, 8}, {6, 8}, {9, 10}, {9, 11}, {9, 12}})
    )
);

// clang-format on

}  // namespace tests
