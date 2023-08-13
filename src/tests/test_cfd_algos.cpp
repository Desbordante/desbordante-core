
// see input_data/cfd_data/LICENSE

#include <filesystem>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/cfd/enums.h"
#include "algorithms/cfd/fd_first_algorithm.h"
#include "config/names.h"
#include "datasets.h"

namespace tests {
namespace fs = std::filesystem;

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
            unsigned minsup, double minconf, const std::filesystem::path& path,
            char const* substrategy, unsigned int max_lhs, unsigned columns_number = 0,
            unsigned tuples_number = 0, char separator = ',', bool hasHeader = true) {
        using namespace config::names;

        algos::StdParamsMap params{
                {kCsvPath, path},
                {kSeparator, separator},
                {kHasHeader, hasHeader},
                {kCfdMinimumSupport, minsup},
                {kCfdMinimumConfidence, minconf},
                {kCfdMaximumLhs, max_lhs},
                {kCfdSubstrategy, algos::cfd::Substrategy::_from_string(substrategy)},
                {kCfdTuplesNumber, tuples_number},
                {kCfdColumnsNumber, columns_number}};
        return algos::CreateAndLoadAlgorithm<algos::cfd::FDFirstAlgorithm>(params);
    }
};

TEST_F(CFDAlgorithmTest, CfdRelationDataStringFormatTest) {
    auto const path = test_data_dir / "cfd_data" / "tennis.csv";
    auto algorithm = CreateAlgorithmInstance(2, 0.85, path, "dfs", 3, 4, 5);
    algorithm->Execute();
    std::string expected_data =
            "outlook temp humidity windy\nsunny hot high false\nsunny hot high true\n";
    expected_data += "overcast hot high false\nrainy mild high false\nrainy cool normal false\n";
    ASSERT_EQ(algorithm->GetRelationString(), expected_data);
}

TEST_F(CFDAlgorithmTest, CfdRelationDataPartialStringFormatTest) {
    auto const path = test_data_dir / "cfd_data" / "tennis.csv";
    auto algorithm = CreateAlgorithmInstance(8, 0.85, path, "dfs", 3);
    algorithm->Execute();
    std::vector<int> tids = {0, 2, 4, 6};
    std::string expected_data =
            "outlook temp humidity windy play\nsunny hot high false no\novercast hot high false "
            "yes\n";
    expected_data += "rainy cool normal false yes\novercast cool normal true yes\n";
    ASSERT_EQ(algorithm->GetRelationString(tids), expected_data);
}

TEST_F(CFDAlgorithmTest, FullTennisDataset) {
    auto const path = test_data_dir / "cfd_data" / "tennis.csv";
    auto algorithm = CreateAlgorithmInstance(8, 0.85, path, "dfs", 3);
    algorithm->Execute();
    std::set<std::string> actual_cfds;
    for (auto const& cfd : algorithm->GetItemsetCfds()) {
        actual_cfds.insert(algorithm->GetCfdString(cfd));
    }
    std::set<std::string> expected_cfds = {"(windy, temp, outlook) => humidity",
                                           "(windy, humidity, outlook) => temp",
                                           "(windy, outlook) => play",
                                           "(outlook, windy=false) => play",
                                           "(windy, temp, outlook) => play",
                                           "(play, temp, outlook) => windy",
                                           "(temp, outlook, play=yes) => windy",
                                           "(play, windy, temp) => outlook",
                                           "(play, temp, windy=false) => outlook",
                                           "(humidity, outlook) => play",
                                           "(humidity, temp, outlook) => play",
                                           "(play, temp, outlook) => humidity",
                                           "(windy, humidity, outlook) => play"};
    CheckCfdSetsEquality(actual_cfds, expected_cfds);

    algorithm = CreateAlgorithmInstance(8, 0.85, path, "bfs", 3);
    algorithm->Execute();
    CheckCfdSetsEquality(actual_cfds, expected_cfds);
}

TEST_F(CFDAlgorithmTest, PartialMushroomDataset) {
    auto const path = test_data_dir / "cfd_data" / "mushroom.csv";
    auto algorithm = CreateAlgorithmInstance(4, 0.9, path, "dfs", 4, 4, 50);
    algorithm->Execute();
    std::set<std::string> actual_cfds;
    for (auto const& cfd : algorithm->GetItemsetCfds()) {
        actual_cfds.insert(algorithm->GetCfdString(cfd));
    }
    std::set<std::string> expected_cfds = {"(edible=p) => cap-shape=x",
                                           "(cap-shape=b) => edible=e",
                                           "(cap-color=y) => edible=e",
                                           "(cap-color, edible=p) => cap-shape",
                                           "(edible=p, cap-color=n) => cap-shape=x",
                                           "(cap-surface=f) => edible=e",
                                           "(cap-color, cap-surface=s) => edible",
                                           "(cap-surface, edible=p) => cap-shape",
                                           "(edible=p, cap-surface=y) => cap-shape=x",
                                           "(cap-surface, cap-shape=f) => edible",
                                           "(cap-shape, edible=p, cap-surface=s) => cap-color",
                                           "(cap-color, edible, cap-shape=f) => cap-surface",
                                           "(cap-shape, edible=p, cap-color=w) => cap-surface",
                                           "(edible=p, cap-shape=x, cap-color=w) => cap-surface=y",
                                           "(cap-color, cap-surface, edible=p) => cap-shape",
                                           "(cap-color, cap-surface, cap-shape) => edible",
                                           "(cap-color, cap-shape, cap-surface=s) => edible",
                                           "(cap-color, cap-surface, cap-shape=x) => edible"};

    CheckCfdSetsEquality(actual_cfds, expected_cfds);
}
}  // namespace tests
