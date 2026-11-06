#include <set>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/aidfd/aid.h"
#include "core/config/names.h"
#include "core/util/bitset_utils.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {
namespace {

using FdSet = std::set<std::pair<std::vector<unsigned int>, unsigned int>>;

CSVConfig const kTestWide66{kTestDataDir / "TestWide66.csv", ',', false};

std::unique_ptr<algos::Aid> CreateAidToLoad(CSVConfig const& csv_config) {
    using namespace config::names;
    using algos::ConfigureFromMap, algos::StdParamsMap;

    auto algorithm = std::make_unique<algos::Aid>();
    ConfigureFromMap(*algorithm, StdParamsMap{{kTable, MakeInputTable(csv_config)}});
    return algorithm;
}

FdSet RunAid(CSVConfig const& csv_config) {
    using namespace config::names;
    auto algorithm = algos::CreateAndLoadAlgorithm<algos::Aid>(
            algos::StdParamsMap{{kCsvConfig, csv_config}});
    algorithm->Execute();

    FdSet result;
    for (auto const& fd : algorithm->FdList()) {
        auto const& raw_fd = fd.ToRawFD();
        result.emplace(util::BitsetToIndices<unsigned int>(raw_fd.lhs_), raw_fd.rhs_);
    }
    return result;
}

}  // namespace

TEST(AidFdTest, ThrowsOnEmpty) {
    auto algorithm = CreateAidToLoad(kTestEmpty);
    ASSERT_THROW(algorithm->LoadData(), std::runtime_error);
}

TEST(AidFdTest, ReturnsEmptyOnSingleNonKey) {
    EXPECT_TRUE(RunAid(kTestSingleColumn).empty());
}

TEST(AidFdTest, WorksOnLongDataset) {
    FdSet expected{{{2}, 1}};
    EXPECT_EQ(RunAid(kTestLong), expected);
}

TEST(AidFdTest, WorksOnWideDatasetWithConstantColumns) {
    FdSet expected{{{0}, 2}, {{0}, 4}, {{2}, 0}, {{2}, 4}, {{4}, 0}, {{4}, 2}, {{}, 1}, {{}, 3}};
    EXPECT_EQ(RunAid(kTestWide), expected);
}

TEST(AidFdTest, WorksOnTableWiderThan64Attributes) {
    // Columns 0..63 are keys, columns 64 and 65 are not keys but their pair is.
    FdSet expected;
    for (unsigned i = 0; i < 64; ++i) {
        for (unsigned j = 0; j < 66; ++j) {
            if (i != j) {
                expected.insert({{i}, j});
            }
        }
        expected.insert({{64, 65}, i});
    }
    EXPECT_EQ(RunAid(kTestWide66), expected);
}

}  // namespace tests
