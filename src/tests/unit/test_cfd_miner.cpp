#include <memory>
#include <set>
#include <string>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/cfd/cfd_miner/cfd_miner.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace tests {

namespace {

std::unique_ptr<algos::cfd::CFDMiner> CreateAlgorithm(CSVConfig const& csv_config, unsigned minsup,
                                                      unsigned max_lhs) {
    using namespace config::names;

    algos::StdParamsMap params{
            {kCsvConfig, csv_config}, {kCfdMinimumSupport, minsup}, {kCfdMaximumLhs, max_lhs}};
    return algos::CreateAndLoadAlgorithm<algos::cfd::CFDMiner>(params);
}

std::set<std::string> ToStrings(algos::cfd::CFDList const& cfds) {
    std::set<std::string> result;
    for (auto const& cfd : cfds) {
        result.insert(cfd.ToString());
    }
    return result;
}

}  // namespace

TEST(CFDMinerTest, MinesExpectedDependenciesFromTennisDataset) {
    auto algorithm = CreateAlgorithm(kTennis, 4, 3);
    algorithm->Execute();

    std::set<std::string> const expected_cfds = {"{(0, overcast)} -> (4, yes)",
                                                 "{(1, cool)} -> (2, normal)",
                                                 "{(2, normal),(3, false)} -> (4, yes)"};
    EXPECT_EQ(ToStrings(algorithm->GetCfds()), expected_cfds);
}

TEST(CFDMinerTest, RespectsMaximumLhsSize) {
    auto algorithm = CreateAlgorithm(kTennis, 4, 1);
    algorithm->Execute();

    std::set<std::string> const expected_cfds = {"{(0, overcast)} -> (4, yes)",
                                                 "{(1, cool)} -> (2, normal)"};
    EXPECT_EQ(ToStrings(algorithm->GetCfds()), expected_cfds);
}

TEST(CFDMinerTest, MinesExpectedDependenciesFromMushroomDataset) {
    auto algorithm = CreateAlgorithm(kMushroom50, 4, 4);
    algorithm->Execute();

    std::set<std::string> const expected_cfds = {"{(0, p),(1, x),(3, w)} -> (2, y)",
                                                 "{(0, p),(3, n)} -> (1, x)",
                                                 "{(0, p),(2, y)} -> (1, x)",
                                                 "{(3, y)} -> (0, e)",
                                                 "{(1, b)} -> (0, e)",
                                                 "{(2, f)} -> (0, e)"};
    EXPECT_EQ(ToStrings(algorithm->GetCfds()), expected_cfds);
}

TEST(CFDMinerTest, ClearsResultsBeforeRepeatedExecution) {
    auto algorithm = CreateAlgorithm(kTennis, 4, 3);
    algorithm->Execute();
    auto const first_result = ToStrings(algorithm->GetCfds());

    algorithm->SetOption(config::names::kCfdMinimumSupport, 4u);
    algorithm->SetOption(config::names::kCfdMaximumLhs, 3u);
    algorithm->Execute();

    EXPECT_EQ(ToStrings(algorithm->GetCfds()), first_result);
}

TEST(CFDMinerTest, RejectsZeroMaximumLhsSize) {
    EXPECT_THROW(CreateAlgorithm(kTennis, 4, 0), config::ConfigurationError);
}

}  // namespace tests
