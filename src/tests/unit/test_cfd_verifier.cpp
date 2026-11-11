#include <optional>
#include <utility>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/cfd/cfd_verifier/cfd_verifier.h"
#include "core/algorithms/cfd/model/raw_cfd.h"
#include "core/config/exceptions.h"
#include "core/config/names_and_descriptions.h"
#include "tests/common/all_csv_configs.h"

namespace tests {

using algos::cfd::RawCFD;
using RawItem = RawCFD::RawItem;
using RawItems = RawCFD::RawItems;

struct CFDVerifierParams {
    algos::StdParamsMap params;
    bool expect_holds;

    CFDVerifierParams(RawCFD rule, CSVConfig const& csv_config = kTennis, double confidence = 1.0,
                      int support = 0, bool expect_holds = true)
        : params({{config::names::kCFDRule, std::move(rule)},
                  {config::names::kCfdMinimumConfidence, confidence},
                  {config::names::kCfdMinimumSupport, support},
                  {config::names::kCsvConfig, csv_config}}),
          expect_holds(expect_holds) {}
};

class CFDVerifierTest : public ::testing::TestWithParam<CFDVerifierParams> {};

class CFDVerifierInvalidAttributeTest : public ::testing::TestWithParam<RawCFD> {};

TEST_P(CFDVerifierInvalidAttributeTest, RejectsOutOfRangeAttribute) {
    algos::cfd_verifier::CFDVerifier verifier;
    algos::LoadAlgorithmData(verifier, {{config::names::kCsvConfig, kTennis}});
    EXPECT_THROW(verifier.SetOption(config::names::kCFDRule, GetParam()),
                 config::ConfigurationError);
}

INSTANTIATE_TEST_SUITE_P(InvalidAttributes, CFDVerifierInvalidAttributeTest,
                         // Tennis has five columns, so valid indices are 0 through 4.
                         ::testing::Values(RawCFD({RawItem{-1, "sunny"}}, RawItem{4, "yes"}),
                                           RawCFD({RawItem{5, std::nullopt}}, RawItem{4, "yes"}),
                                           RawCFD({RawItem{-1, "_"}}, RawItem{4, "yes"}),
                                           RawCFD({}, RawItem{5, "yes"}),
                                           RawCFD({}, RawItem{-1, std::nullopt}),
                                           RawCFD({}, RawItem{5, "_"})));

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
                CFDVerifierParams(RawCFD({RawItem{0, "overcast"}},  // LHS: [(outlook=overcast)]
                                         RawItem{4, "yes"}),        // RHS: (play=yes)
                                  kTennis, 1.0, 0, true),

                // 2. Negative tests (CFD is violated)
                CFDVerifierParams(RawCFD({RawItem{1, "mild"}},  // LHS: [(temp=mild)]
                                         RawItem{4, "yes"}),    // RHS: (play=yes)
                                  kTennis, 1.0, 0, false),
                CFDVerifierParams(RawCFD({RawItem{0, "rainy"}},  // LHS: [(outlook=rainy)]
                                         RawItem{4, "yes"}),     // RHS: (play=yes)
                                  kTennis, 1.0, 0, false),
                CFDVerifierParams(RawCFD({RawItem{1, "cool"}},  // LHS: [(temp=cool)]
                                         RawItem{4, "yes"}),    // RHS: (play=yes)
                                  kTennis, 1.0, 0, false),

                // 3. Tests with confidence
                CFDVerifierParams(RawCFD({RawItem{2, "normal"}},  // LHS: [(humidity=normal)]
                                         RawItem{4, "yes"}),      // RHS: (play=yes)
                                  kTennis, 0.8, 0, true),
                CFDVerifierParams(RawCFD({RawItem{2, "normal"}},  // LHS: [(humidity=normal)]
                                         RawItem{4, "yes"}),      // RHS: (play=yes)
                                  kTennis, 1.0, 0, false),

                // 4. Boundary cases
                CFDVerifierParams(RawCFD({},  // Empty LHS (should work as `true` by default)
                                         RawItem{4, "yes"}),  // RHS: (play=yes)
                                  kTennis, 1.0, 0, false),

                // 5. Tests with wildcards
                CFDVerifierParams(RawCFD({RawItem{1, std::nullopt},  // temp=_
                                          RawItem{0, std::nullopt},  // outlook=_
                                          RawItem{4, "yes"}},        // play=yes
                                         RawItem{3, std::nullopt}),  // RHS: (windy=_)
                                  kTennis, 1.0, 0, true),
                CFDVerifierParams(RawCFD({RawItem{1, "_"},     // temp=_
                                          RawItem{0, "_"},     // outlook=_
                                          RawItem{4, "yes"}},  // play=yes
                                         RawItem{3, "_"}),     // RHS: (windy=_)
                                  kTennis, 1.0, 0, true)));

}  // namespace tests
