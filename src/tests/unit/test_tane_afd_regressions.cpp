#include <gtest/gtest.h>

#include "test_tane_afd_util.h"

namespace tests {

TEST(TestTanePrimeAfdMeasures, G1PrimeUsesFullTupleMultiplicity) {
    auto tane = RunTane(algos::AfdErrorMeasure::kG1Prime, 1.0 / 63.0 + kTaneTolerance, 1, kTestFD);
    ::AFD const* afd = FindAfd(*tane, {4}, 3);
    ASSERT_NE(afd, nullptr);
    EXPECT_NEAR(afd->GetThreshold(), 1.0 / 63.0, kTaneTolerance);
}

TEST(TestTaneAfdBehavior, G1RejectsZeroAryDependenciesForOneRow) {
    CSVConfig const one_row{kTestDataDir / "RfiPlusOneRow.csv", ',', true};
    // With zero tuple pairs, G1 returns positive infinity even for a constant RHS.
    // No dependency passes the maximum supported threshold.
    auto tane = RunTane(algos::AfdErrorMeasure::kG1, 1.0, 0, one_row);
    EXPECT_TRUE(tane->AfdList().empty());
}

TEST(TestTaneAfdBehavior, G1RejectsOrdinaryDependenciesForOneRow) {
    CSVConfig const one_row{kTestDataDir / "RfiPlusOneRow.csv", ',', true};
    // With no zero-ary dependencies accepted, ordinary candidates are still evaluated.
    // Their G1 errors are also positive infinity, rather than overridden to zero by exactness.
    auto tane = RunTane(algos::AfdErrorMeasure::kG1, 1.0, 1, one_row);
    EXPECT_TRUE(tane->AfdList().empty());
}

}  // namespace tests
