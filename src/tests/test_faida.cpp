#include <filesystem>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/ind/faida/faida.h"
#include "all_tables_config.h"
#include "config/names.h"
#include "table_config.h"

namespace fs = std::filesystem;

namespace tests {

using IndCCTest = std::pair<model::TableIndex, std::vector<model::ColumnIndex>>;
using IndsTest = std::set<std::pair<IndCCTest, IndCCTest>>;
using OrderedDatasetsTest = std::vector<std::string>;

void CheckINDsListsEquality(std::list<model::IND> const& actual, IndsTest const& expected) {
    ASSERT_EQ(actual.size(), expected.size())
            << "count of generated dependencies does not match: expected " << expected.size()
            << ", got " << actual.size();

    auto to_test_cc = [](model::ColumnCombination const& cc) -> IndCCTest {
        return std::make_pair(cc.GetTableIndex(), cc.GetColumnIndices());
    };

    for (auto const& dep : actual) {
        if (expected.find(std::make_pair(to_test_cc(dep.GetLhs()), to_test_cc(dep.GetRhs()))) ==
            expected.end()) {
            FAIL() << "generated dep '" << dep.ToString() << "' is not present in expected";
        }
    }
    SUCCEED();
}

void CheckResultContainsINDs(std::list<model::IND> const& actual, IndsTest expected_subset) {
    ASSERT_NE(actual.size(), expected_subset.size())
            << "count of generated dependencies must not not be equal to the subset size: got "
            << actual.size();

    auto to_test_cc = [](model::ColumnCombination const& cc) -> IndCCTest {
        return std::make_pair(cc.GetTableIndex(), cc.GetColumnIndices());
    };

    for (auto const& dep : actual) {
        auto iter = expected_subset.find(
                std::make_pair(to_test_cc(dep.GetLhs()), to_test_cc(dep.GetRhs())));

        if (iter != expected_subset.end()) {
            expected_subset.erase(iter);
        }
    }
    ASSERT_EQ(expected_subset.size(), 0);
}

// Since Faida is an approximate algorithm, its results may differ from all the other IND discovery
// algorithms, which are exact. Of course, on these small examples the results of both exact and
// approximate algos must be the same, but in the future the set of test datasets will grow and
// large tests may be added. Therefore the decision was made to move Faida tests to a separate
// class.
class FaidaINDAlgorithmTest : public ::testing::Test {
protected:
    static algos::StdParamsMap GetParamMap(std::vector<fs::path> const& paths, char separator = ',',
                                           bool has_header = true, int sample_size = 500,
                                           double hll_accuracy = 0.001, bool find_nary = true,
                                           unsigned short num_threads = 4) {
        using namespace config::names;
        return {{kCsvPaths, paths},     {kSeparator, separator},    {kHasHeader, has_header},
                {kFindNary, find_nary}, {kSampleSize, sample_size}, {kHllAccuracy, hll_accuracy},
                {kThreads, num_threads}};
    }

    template <typename... Args>
    static std::unique_ptr<algos::INDAlgorithm> CreateFaidaInstance(Args&&... args) {
        return algos::CreateAndLoadAlgorithm<algos::Faida>(
                GetParamMap(std::forward<Args>(args)...));
    }
};

TEST_F(FaidaINDAlgorithmTest, TestWide2) {
    IndsTest expected_inds{{{0, {2}}, {0, {0}}}, {{0, {3}}, {0, {1}}}, {{0, {2, 3}}, {0, {0, 1}}}};

    TableConfig const& table = kIndTestWide2;

    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    auto algorithm =
            CreateFaidaInstance(std::vector{table.GetPath()}, table.separator, table.has_header,
                                sample_size, hll_accuracy, find_nary, num_threads);
    algorithm->Execute();
    CheckINDsListsEquality(algorithm->INDList(), expected_inds);
}

TEST_F(FaidaINDAlgorithmTest, TestEmpty) {
    IndsTest expected_inds{};

    TableConfig const& table = kIndTestEmpty;

    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    ASSERT_THROW(
            CreateFaidaInstance(std::vector{table.GetPath()}, table.separator, table.has_header,
                                sample_size, hll_accuracy, find_nary, num_threads),
            std::runtime_error);
}

TEST_F(FaidaINDAlgorithmTest, TestEmptyInput) {
    char separator = ',';
    bool has_header = false;
    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    ASSERT_THROW(CreateFaidaInstance(std::vector<fs::path>{}, separator, has_header, sample_size,
                                     hll_accuracy, find_nary, num_threads),
                 config::ConfigurationError);
}

TEST_F(FaidaINDAlgorithmTest, TestPlanets) {
    IndsTest expected_inds{{{0, {0}}, {0, {1}}},       {{0, {1}}, {0, {0}}},
                           {{0, {2}}, {0, {3}}},       {{0, {3}}, {0, {2}}},
                           {{0, {1, 3}}, {0, {0, 2}}}, {{0, {0, 2}}, {0, {1, 3}}},
                           {{0, {0, 3}}, {0, {1, 2}}}, {{0, {1, 2}}, {0, {0, 3}}}};

    TableConfig const& table = kIndTestPlanets;

    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    auto algorithm =
            CreateFaidaInstance(std::vector{table.GetPath()}, table.separator, table.has_header,
                                sample_size, hll_accuracy, find_nary, num_threads);
    algorithm->Execute();
    CheckINDsListsEquality(algorithm->INDList(), expected_inds);
}

TEST_F(FaidaINDAlgorithmTest, Test3ary) {
    IndsTest expected_inds{{{0, {3}}, {0, {0}}},
                           {{0, {4}}, {0, {1}}},
                           {{0, {5}}, {0, {2}}},
                           {{0, {2}}, {0, {5}}},
                           {{0, {3, 4}}, {0, {0, 1}}},
                           {{0, {4, 5}}, {0, {1, 2}}},
                           {{0, {3, 5}}, {0, {0, 2}}},
                           {{0, {3, 4, 5}}, {0, {0, 1, 2}}},
                           {{0, {3, 4, 5}}, {0, {0, 1, 2}}}};

    TableConfig const& table = kIndTest3aryInds;

    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    auto algorithm =
            CreateFaidaInstance(std::vector{table.GetPath()}, table.separator, table.has_header,
                                sample_size, hll_accuracy, find_nary, num_threads);
    algorithm->Execute();
    CheckINDsListsEquality(algorithm->INDList(), expected_inds);
}

TEST_F(FaidaINDAlgorithmTest, TestTwoTables) {
    IndsTest expected_inds_subset{{{0, {0, 1, 2, 3}}, {1, {0, 1, 3, 4}}},
                                  {{1, {0, 1, 3, 4}}, {0, {0, 1, 2, 3}}}};
    size_t constexpr expected_result_size = 47;

    TableConfig const& table1 = kIndTestTableFirst;
    TableConfig const& table2 = kIndTestTableSecond;

    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    std::vector file_paths{table1.GetPath(), table2.GetPath()};

    auto algorithm = CreateFaidaInstance(file_paths, table1.separator, table1.has_header,
                                         sample_size, hll_accuracy, find_nary, num_threads);
    algorithm->Execute();
    auto result = algorithm->INDList();

    ASSERT_EQ(result.size(), expected_result_size);
    CheckResultContainsINDs(result, expected_inds_subset);
}

}  // namespace tests
