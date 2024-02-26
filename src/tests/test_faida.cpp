#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/ind/faida/faida.h"
#include "all_csv_configs.h"
#include "config/names.h"
#include "test_ind_util.h"

namespace fs = std::filesystem;

namespace tests {

using INDTestSet = std::set<INDTest>;

void CheckINDsListsEquality(std::list<model::IND> const& actual, INDTestSet const& expected) {
    ASSERT_EQ(actual.size(), expected.size())
            << "count of generated dependencies does not match: expected " << expected.size()
            << ", got " << actual.size();

    for (auto const& dep : actual) {
        if (expected.find(ToINDTest(dep)) == expected.end()) {
            FAIL() << "generated dep '" << dep.ToString() << "' is not present in expected";
        }
    }
    SUCCEED();
}

void CheckResultContainsINDs(std::list<model::IND> const& actual, INDTestSet expected_subset) {
    ASSERT_NE(actual.size(), expected_subset.size())
            << "count of generated dependencies must not not be equal to the subset size: got "
            << actual.size();

    for (auto const& dep : actual) {
        auto iter = expected_subset.find(ToINDTest(dep));

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
    static algos::StdParamsMap GetParamMap(CSVConfigs const& csv_configs, int sample_size,
                                           double hll_accuracy, bool find_nary,
                                           unsigned short num_threads) {
        using namespace config::names;
        return {{kCsvConfigs, csv_configs},
                {kFindNary, find_nary},
                {kSampleSize, sample_size},
                {kHllAccuracy, hll_accuracy},
                {kThreads, num_threads}};
    }

    template <typename... Args>
    static std::unique_ptr<algos::INDAlgorithm> CreateFaidaInstance(Args&&... args) {
        return algos::CreateAndLoadAlgorithm<algos::Faida>(
                GetParamMap(std::forward<Args>(args)...));
    }
};

TEST_F(FaidaINDAlgorithmTest, TestWide2) {
    INDTestSet expected_inds{
            {{0, {2}}, {0, {0}}}, {{0, {3}}, {0, {1}}}, {{0, {2, 3}}, {0, {0, 1}}}};

    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    auto algorithm = CreateFaidaInstance(CSVConfigs{kIndTestWide2}, sample_size, hll_accuracy,
                                         find_nary, num_threads);
    algorithm->Execute();
    CheckINDsListsEquality(algorithm->INDList(), expected_inds);
}

TEST_F(FaidaINDAlgorithmTest, TestEmpty) {
    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    ASSERT_THROW(CreateFaidaInstance(CSVConfigs{kIndTestEmpty}, sample_size, hll_accuracy,
                                     find_nary, num_threads),
                 std::runtime_error);
}

TEST_F(FaidaINDAlgorithmTest, TestEmptyInput) {
    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    ASSERT_THROW(
            CreateFaidaInstance(CSVConfigs{}, sample_size, hll_accuracy, find_nary, num_threads),
            config::ConfigurationError);
}

TEST_F(FaidaINDAlgorithmTest, TestPlanets) {
    INDTestSet expected_inds{{{0, {0}}, {0, {1}}},       {{0, {1}}, {0, {0}}},
                             {{0, {2}}, {0, {3}}},       {{0, {3}}, {0, {2}}},
                             {{0, {1, 3}}, {0, {0, 2}}}, {{0, {0, 2}}, {0, {1, 3}}},
                             {{0, {0, 3}}, {0, {1, 2}}}, {{0, {1, 2}}, {0, {0, 3}}}};

    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    auto algorithm = CreateFaidaInstance(CSVConfigs{kIndTestPlanets}, sample_size, hll_accuracy,
                                         find_nary, num_threads);
    algorithm->Execute();
    CheckINDsListsEquality(algorithm->INDList(), expected_inds);
}

TEST_F(FaidaINDAlgorithmTest, Test3ary) {
    INDTestSet expected_inds{{{0, {3}}, {0, {0}}},
                             {{0, {4}}, {0, {1}}},
                             {{0, {5}}, {0, {2}}},
                             {{0, {2}}, {0, {5}}},
                             {{0, {3, 4}}, {0, {0, 1}}},
                             {{0, {4, 5}}, {0, {1, 2}}},
                             {{0, {3, 5}}, {0, {0, 2}}},
                             {{0, {3, 4, 5}}, {0, {0, 1, 2}}},
                             {{0, {3, 4, 5}}, {0, {0, 1, 2}}}};

    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    auto algorithm = CreateFaidaInstance(CSVConfigs{kIndTest3aryInds}, sample_size, hll_accuracy,
                                         find_nary, num_threads);
    algorithm->Execute();
    CheckINDsListsEquality(algorithm->INDList(), expected_inds);
}

TEST_F(FaidaINDAlgorithmTest, TestTwoTables) {
    INDTestSet expected_inds_subset{{{0, {0, 1, 2, 3}}, {1, {0, 1, 3, 4}}},
                                    {{1, {0, 1, 3, 4}}, {0, {0, 1, 2, 3}}}};
    size_t constexpr expected_result_size = 47;

    int sample_size = 500;
    double hll_accuracy = 0.001;
    bool find_nary = true;
    unsigned short num_threads = 4;

    auto algorithm = CreateFaidaInstance(CSVConfigs{kIndTestTableFirst, kIndTestTableSecond},
                                         sample_size, hll_accuracy, find_nary, num_threads);
    algorithm->Execute();
    auto result = algorithm->INDList();

    ASSERT_EQ(result.size(), expected_result_size);
    CheckResultContainsINDs(result, expected_inds_subset);
}

}  // namespace tests
