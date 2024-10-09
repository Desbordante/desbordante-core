#include "algorithms/algo_factory.h"
#include "all_csv_configs.h"
#include "dc/verifier/dc_verifier.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "names_and_descriptions.h"

namespace tests {

using namespace algos;
using namespace algos::dc;

namespace mo = model;

static algos::StdParamsMap GetParamMap(CSVConfig const &csv_config, std::string dc) {
    using namespace config::names;
    return {{kCsvConfig, csv_config}, {kDenialConstraint, dc}};
}

// Unique column combination
TEST(TestDCVerifier, TestUCC) {
    std::string dc_string = "!(t.Col3 == s.Col3 and s.Col1 == t.Col1 and s.Col2 == t.Col2)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC, dc_string));

    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestNotUCC) {
    std::string dc_string = "!(t.Col1 == s.Col1 and s.Col2 == t.Col2 and s.Col0 == t.Col0)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC, dc_string));

    dc_verifier->Execute();
    EXPECT_FALSE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestOneInequalityOnInts) {
    std::string dc_string = "!(s.Col0 == t.Col0 and t.Col1 == s.Col1 and s.Col2 > t.Col4)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestColNums) {
    std::string dc_string = "!(s.0 == t.0 and t.1 == s.1 and s.2 > t.4)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestMixedNamesColNums) {
    std::string dc_string = "!(s.0 == t.0 and t.Col1 == s.Col1 and s.Col2 > t.4)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestNoHeader) {
    std::string dc_string = "!(s.0 == t.1 and s.1 == t.2 and s.2 == t.3)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kBernoulliRelation, dc_string));
    dc_verifier->Execute();
    EXPECT_FALSE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestOneInequalityOnDoubles) {
    std::string dc_string = "!(s.Col0 == t.Col0 and s.Col5 <= t.Col6)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestOneInequalityOnStrings) {
    std::string dc_string = "!(t.Col7 > s.Col3 and s.Col1 == t.Col1)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestOneInequalityOnDiffTypes) {
    std::string dc_string = "!(t.Col2 == s.Col2 and s.Col4 >= t.Col5)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestRowHomogeneousInequalities) {
    std::string dc_string =
            "!(s.Salary < t.Salary and s.State == t.State and s.FedTaxRate > t.FedTaxRate)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC1, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestRowHomogeneousInequalitiesV2) {
    std::string dc_string =
            "!(s.Salary <= t.Salary and s.State == t.State and s.FedTaxRate >= t.FedTaxRate)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC1, dc_string));
    dc_verifier->Execute();
    EXPECT_FALSE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestRowHeterogeneousInequalities) {
    std::string dc_string = "!(s.Salary > t.FedTaxRate and s.Salary <= t.FedTaxRate)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC1, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestMixedEqualities) {
    std::string dc_string = "!(s.Salary == t.FedTaxRate)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC1, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestOneTuple) {
    std::string dc_string = "!(s.Salary < s.Salary and t.State != t.State)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC1, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestOneTupleV2) {
    std::string dc_string =
            "!(t.Salary != t.FedTaxRate and s.Salary < s.FedTaxRate and t.State == t.State)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC1, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestMixedDC) {
    std::string dc_string =
            "!(t.Salary == t.FedTaxRate and s.Salary == s.FedTaxRate and s.Salary == t.FedTaxRate)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC1, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestMixedEquality) {
    std::string dc_string = "!(t.Salary == s.FedTaxRate and s.Salary == t.FedTaxRate)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC1, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerifier, TestMixedDisequality) {
    std::string dc_string = "!(t.Salary != s.FedTaxRate and s.Salary != t.FedTaxRate)";
    std::unique_ptr<DCVerifier> dc_verifier =
            algos::CreateAndLoadAlgorithm<DCVerifier>(GetParamMap(kTestDC1, dc_string));
    dc_verifier->Execute();
    EXPECT_FALSE(dc_verifier->DCHolds());
}

}  // namespace tests
