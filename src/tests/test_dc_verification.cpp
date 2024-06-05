#include "../core/algorithms/dc/dc_verification.h"
#include "algorithms/algo_factory.h"
#include "all_csv_configs.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "names_and_descriptions.h"

namespace tests {

namespace mo = model;

static algos::StdParamsMap GetParamMap(CSVConfig const &csv_config, std::string dc) {
    using namespace config::names;
    return {{kCsvConfig, csv_config}, {kDenialConstraint, dc}};
}

TEST(TestDCVerification, TestUCC) {
    std::string dc_string = "!(t.Col0 == s.Col0 and t.Col1 == s.Col1)";
    std::unique_ptr<algos::DCVerification> dc_verifier =
            algos::CreateAndLoadAlgorithm<algos::DCVerification>(GetParamMap(kTestFD, dc_string));

    dc_verifier->Execute();
    LOG(INFO) << dc_verifier->DCHolds();
}

}  // namespace tests