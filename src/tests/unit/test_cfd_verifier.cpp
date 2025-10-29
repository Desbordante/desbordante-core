#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/container/allocator_traits.hpp>
#include <gtest/gtest.h>

#include "algo_factory.h"
#include "algorithms/cfd/cfd_verifier/cfd_verifier.h"
#include "all_csv_configs.h"
#include "config/names_and_descriptions.h"

struct CSVConfig;

namespace tests {

struct CFDVerifierParams {
    algos::StdParamsMap params;
    bool expect_holds;

    CFDVerifierParams(std::vector<std::pair<std::string, std::string>> left,
                      std::pair<std::string, std::string> right,
                      CSVConfig const& csv_config = kTennis, double confidence = 1.0,
                      int support = 0, bool expect_holds = true)
        : params({{config::names::kCFDRuleLeft, std::move(left)},
                  {config::names::kCFDRuleRight, std::move(right)},
                  {config::names::kMinimumConfidence, confidence},
                  {config::names::kMinimumSupport, support},
                  {config::names::kCsvConfig, csv_config}}),
          expect_holds(expect_holds) {}
};

class CFDVerifierTest : public ::testing::TestWithParam<CFDVerifierParams> {};

TEST_P(CFDVerifierTest, Test) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto verifier = algos::CreateAndLoadAlgorithm<algos::cfd_verifier::CFDVerifier>(mp);
    verifier->Execute();

    EXPECT_EQ(verifier->CFDHolds(), p.expect_holds);
}

INSTANTIATE_TEST_SUITE_P(
        CFDVerifierAdditionalTests, CFDVerifierTest,
        ::testing::Values(
                // 1. Positive tests (CFD is being performed)
                CFDVerifierParams({{"outlook", "overcast"}},  // LHS
                                  {"play", "yes"},            // RHS
                                  kTennis, 1.0, 0, true),
                CFDVerifierParams({{"temp", "_"}, {"outlook", "_"}, {"play", "yes"}},  // LHS
                                  {"windy", "_"},  // RHS (wildcard)
                                  kTennis, 1.0, 0, true),

                // 2. Negative tests (CFD is violated)
                CFDVerifierParams({{"temp", "mild"}},  // LHS
                                  {"play", "yes"},     // RHS
                                  kTennis, 1.0, 0, false),
                CFDVerifierParams({{"outlook", "rainy"}},  // LHS
                                  {"play", "yes"},         // RHS
                                  kTennis, 1.0, 0, false),
                CFDVerifierParams({{"temp", "cool"}},  // LHS
                                  {"play", "yes"},     // RHS
                                  kTennis, 1.0, 0, false),

                // 3. Tests with confidence
                CFDVerifierParams({{"humidity", "normal"}},  // LHS
                                  {"play", "yes"},           // RHS
                                  kTennis, 0.8, 0, true),
                CFDVerifierParams({{"humidity", "normal"}},  // LHS
                                  {"play", "yes"},           // RHS
                                  kTennis, 1.0, 0,           // At 1.0 it is violated
                                  false),

                // 4. Boundary cases
                CFDVerifierParams({},               // Empty LHS (should work as `true` by default)
                                  {"play", "yes"},  // RHS
                                  kTennis, 1.0, 0, false)));

}  // namespace tests
