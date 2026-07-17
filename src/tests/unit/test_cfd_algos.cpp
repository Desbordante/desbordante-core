
// see input_data/cfd_data/LICENSE

#include <gtest/gtest.h>
#include <magic_enum/magic_enum.hpp>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/cfd/enums.h"
#include "core/algorithms/cfd/fd_first_algorithm.h"
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

class CFDAlgorithmTest : public ::testing::Test {
protected:
    static std::unique_ptr<algos::cfd::FDFirstAlgorithm> CreateAlgorithmInstance(
            CSVConfig const& csv_config, unsigned minsup, double minconf,
            algos::cfd::Substrategy substrategy, unsigned int max_lhs) {
        using namespace config::names;

        algos::StdParamsMap params{{kCsvConfig, csv_config},
                                   {kCfdMinimumSupport, minsup},
                                   {kCfdMinimumConfidence, minconf},
                                   {kCfdMaximumLhs, max_lhs},
                                   {kCfdSubstrategy, substrategy}};
        return algos::CreateAndLoadAlgorithm<algos::cfd::FDFirstAlgorithm>(params);
    }
};

TEST_F(CFDAlgorithmTest, FullTennisDataset) {
    auto algorithm = CreateAlgorithmInstance(kTennis, 8, 0.85, algos::cfd::Substrategy::kDfs, 3);
    algorithm->Execute();
    std::set<std::string> actual_cfds;
    for (auto const& cfd : algorithm->GetCfds()) {
        actual_cfds.insert(cfd.ToString());
    }
    std::set<std::string> expected_cfds = {"{(3, _),(1, _),(0, _)} -> (2, _)",
                                           "{(3, _),(2, _),(0, _)} -> (1, _)",
                                           "{(3, _),(0, _)} -> (4, _)",
                                           "{(0, _),(3, false)} -> (4, _)",
                                           "{(3, _),(1, _),(0, _)} -> (4, _)",
                                           "{(4, _),(1, _),(0, _)} -> (3, _)",
                                           "{(1, _),(0, _),(4, yes)} -> (3, _)",
                                           "{(4, _),(3, _),(1, _)} -> (0, _)",
                                           "{(4, _),(1, _),(3, false)} -> (0, _)",
                                           "{(2, _),(0, _)} -> (4, _)",
                                           "{(2, _),(1, _),(0, _)} -> (4, _)",
                                           "{(4, _),(1, _),(0, _)} -> (2, _)",
                                           "{(3, _),(2, _),(0, _)} -> (4, _)"};
    CheckCfdSetsEquality(actual_cfds, expected_cfds);

    algorithm = CreateAlgorithmInstance(kTennis, 8, 0.85, algos::cfd::Substrategy::kBfs, 3);
    algorithm->Execute();
    CheckCfdSetsEquality(actual_cfds, expected_cfds);
}

TEST_F(CFDAlgorithmTest, PartialMushroomDataset) {
    auto algorithm = CreateAlgorithmInstance(kMushroom50, 4, 0.9, algos::cfd::Substrategy::kDfs, 4);
    algorithm->Execute();
    std::set<std::string> actual_cfds;
    for (auto const& cfd : algorithm->GetCfds()) {
        actual_cfds.insert(cfd.ToString());
    }
    std::set<std::string> expected_cfds = {"{(0, p)} -> (1, x)",
                                           "{(1, b)} -> (0, e)",
                                           "{(3, y)} -> (0, e)",
                                           "{(3, _),(0, p)} -> (1, _)",
                                           "{(0, p),(3, n)} -> (1, x)",
                                           "{(2, f)} -> (0, e)",
                                           "{(3, _),(2, s)} -> (0, _)",
                                           "{(2, _),(0, p)} -> (1, _)",
                                           "{(0, p),(2, y)} -> (1, x)",
                                           "{(2, _),(1, f)} -> (0, _)",
                                           "{(1, _),(0, p),(2, s)} -> (3, _)",
                                           "{(3, _),(0, _),(1, f)} -> (2, _)",
                                           "{(1, _),(0, p),(3, w)} -> (2, _)",
                                           "{(0, p),(1, x),(3, w)} -> (2, y)",
                                           "{(3, _),(2, _),(0, p)} -> (1, _)",
                                           "{(3, _),(2, _),(1, _)} -> (0, _)",
                                           "{(3, _),(1, _),(2, s)} -> (0, _)",
                                           "{(3, _),(2, _),(1, x)} -> (0, _)"};

    CheckCfdSetsEquality(actual_cfds, expected_cfds);
}
}  // namespace tests
