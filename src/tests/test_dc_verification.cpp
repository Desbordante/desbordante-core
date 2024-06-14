#include "../core/algorithms/dc/dc_verification.h"
#include "algorithms/algo_factory.h"
#include "all_csv_configs.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "names_and_descriptions.h"

namespace tests {

namespace mo = model;

static CSVConfig const kTestDC{"input_data/TestDC.csv", ',', true};

static algos::StdParamsMap GetParamMap(CSVConfig const &csv_config, std::string dc) {
    using namespace config::names;
    return {{kCsvConfig, csv_config}, {kDenialConstraint, dc}};
}

// UCC stands for unique column combination
TEST(TestDCVerification, TestUCC) {
    std::string dc_string = "!(t.Col3 == s.Col3 and s.Col1 == t.Col1 and s.Col2 == t.Col2)";
    std::unique_ptr<algos::DCVerification> dc_verifier =
            algos::CreateAndLoadAlgorithm<algos::DCVerification>(GetParamMap(kTestDC, dc_string));

    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerification, TestNotUCC) {
    std::string dc_string = "!(t.Col1 == s.Col1 and s.Col2 == t.Col2 and s.Col0 == t.Col0)";
    std::unique_ptr<algos::DCVerification> dc_verifier =
            algos::CreateAndLoadAlgorithm<algos::DCVerification>(GetParamMap(kTestDC, dc_string));

    dc_verifier->Execute();
    EXPECT_FALSE(dc_verifier->DCHolds());
}

TEST(TestDCVerification, TestOneInequalityOnInts) {
    std::string dc_string = "!(s.Col0 == t.Col0 and t.Col1 == s.Col1 and s.Col2 > t.Col4)";
    std::unique_ptr<algos::DCVerification> dc_verifier =
            algos::CreateAndLoadAlgorithm<algos::DCVerification>(GetParamMap(kTestDC, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());    
}

TEST(TestDCVerification, TestOneInequalityOnDoubles) {
    std::string dc_string = "!(s.Col0 == t.Col0 and s.Col5 <= t.Col6)";
    std::unique_ptr<algos::DCVerification> dc_verifier =
            algos::CreateAndLoadAlgorithm<algos::DCVerification>(GetParamMap(kTestDC, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

TEST(TestDCVerification, TestOneInequalityOnStrings) {
    std::string dc_string = "!(s.Col3 < t.Col7 and s.Col1 == t.Col1)";
    std::unique_ptr<algos::DCVerification> dc_verifier =
            algos::CreateAndLoadAlgorithm<algos::DCVerification>(GetParamMap(kTestDC, dc_string));
    dc_verifier->Execute();
    EXPECT_TRUE(dc_verifier->DCHolds());
}

}  // namespace tests