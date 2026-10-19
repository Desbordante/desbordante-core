#include <memory>
#include <set>
#include <string>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/cfd/ctane/ctane.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace tests {

static void CheckCfdSetsEquality(std::set<std::string> const& actual,
                                 std::set<std::string> const& expected) {
    ASSERT_EQ(actual.size(), expected.size()) << "count of cfds does not match: expected "
                                              << expected.size() << ", got " << actual.size();

    for (auto const& string_cfd : actual) {
        if (expected.find(string_cfd) == expected.end()) {
            FAIL() << "generated cfd not found in expected";
        }
    }
    SUCCEED();
}

class CTaneAlgorithmTest : public ::testing::Test {
protected:
    static std::unique_ptr<algos::cfd::CTaneAlgorithm> CreateAlgorithmInstance(
            CSVConfig const& csv_config, unsigned minsup, double minconf, unsigned int max_lhs) {
        using namespace config::names;

        algos::StdParamsMap params{{kCsvConfig, csv_config},
                                   {kCfdMinimumSupport, minsup},
                                   {kCfdMinimumConfidence, minconf},
                                   {kCfdMaximumLhs, max_lhs}};
        return algos::CreateAndLoadAlgorithm<algos::cfd::CTaneAlgorithm>(params);
    }
};

TEST_F(CTaneAlgorithmTest, FullTennisDataset) {
    auto algorithm = CreateAlgorithmInstance(kTennis, 8, 0.85, 3);
    algorithm->Execute();
    std::set<std::string> actual_cfds;
    for (auto const& cfd : algorithm->GetCfds()) {
        actual_cfds.insert(cfd.ToString());
    }
    std::set<std::string> expected_cfds = {"{(0, _),(1, _),(3, _)} -> (2, _)",
                                           "{(0, _),(2, _),(3, _)} -> (1, _)",
                                           "{(0, _),(3, _)} -> (4, _)",
                                           "{(0, _),(3, false)} -> (4, _)",
                                           "{(0, _),(1, _),(3, _)} -> (4, _)",
                                           "{(0, _),(1, _),(4, _)} -> (3, _)",
                                           "{(0, _),(1, _),(4, yes)} -> (3, _)",
                                           "{(1, _),(3, _),(4, _)} -> (0, _)",
                                           "{(1, _),(3, false),(4, _)} -> (0, _)",
                                           "{(0, _),(2, _)} -> (4, _)",
                                           "{(0, _),(1, _),(2, _)} -> (4, _)",
                                           "{(0, _),(1, _),(4, _)} -> (2, _)",
                                           "{(0, _),(2, _),(3, _)} -> (4, _)"};
    CheckCfdSetsEquality(actual_cfds, expected_cfds);
}

TEST_F(CTaneAlgorithmTest, PartialMushroomDataset) {
    auto algorithm = CreateAlgorithmInstance(kMushroom50, 4, 0.9, 4);
    algorithm->Execute();
    std::set<std::string> actual_cfds;
    for (auto const& cfd : algorithm->GetCfds()) {
        actual_cfds.insert(cfd.ToString());
    }
    // There are some CFDs where one of the columns in the LHS can be dropped,
    // because CTANE does not perform pruning for non‑100% CFDs
    std::set<std::string> expected_cfds = {"{(1, b)} -> (0, _)",
                                           "{(2, f)} -> (0, _)",
                                           "{(3, y)} -> (0, _)",
                                           "{(0, p)} -> (1, _)",
                                           "{(0, p)} -> (1, x)",
                                           "{(1, b)} -> (0, e)",
                                           "{(2, f)} -> (0, e)",
                                           "{(3, y)} -> (0, e)",
                                           "{(0, p),(2, _)} -> (1, _)",
                                           "{(0, p),(2, y)} -> (1, _)",
                                           "{(0, p),(3, _)} -> (1, _)",
                                           "{(0, p),(3, n)} -> (1, _)",
                                           "{(1, f),(2, _)} -> (0, _)",
                                           "{(0, p),(2, _)} -> (1, x)",
                                           "{(0, p),(2, y)} -> (1, x)",
                                           "{(0, p),(3, _)} -> (1, x)",
                                           "{(0, p),(3, n)} -> (1, x)",
                                           "{(2, s),(3, _)} -> (0, _)",
                                           "{(1, _),(2, _),(3, _)} -> (0, _)",
                                           "{(0, _),(1, f),(3, _)} -> (2, _)",
                                           "{(1, _),(2, s),(3, _)} -> (0, _)",
                                           "{(0, p),(1, _),(3, w)} -> (2, _)",
                                           "{(0, p),(2, _),(3, _)} -> (1, _)",
                                           "{(0, p),(1, _),(2, s)} -> (3, _)",
                                           "{(1, x),(2, _),(3, _)} -> (0, _)",
                                           "{(0, p),(1, x),(3, w)} -> (2, y)",
                                           "{(0, p),(2, _),(3, _)} -> (1, x)"};

    CheckCfdSetsEquality(actual_cfds, expected_cfds);
}
}  // namespace tests
