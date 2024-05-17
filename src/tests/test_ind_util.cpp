#include "test_ind_util.h"

#include <algorithm>
#include <sstream>

#include <gtest/gtest.h>

#include "all_csv_configs.h"
#include "ind/ind_algorithm.h"

namespace tests {

std::string TableNamesToString(CSVConfigs const& csv_configs) {
    std::stringstream ss;
    for (auto const& csv_config : csv_configs) {
        ss << csv_config.path.filename() << " ";
    }
    return ss.str();
}

INDTest ToINDTest(model::IND const& ind) {
    auto to_cc_test = [](model::ColumnCombination const& cc) {
        return std::make_pair(cc.GetTableIndex(), cc.GetColumnIndices());
    };
    return std::make_pair(to_cc_test(ind.GetLhs()), to_cc_test(ind.GetRhs()));
}

std::vector<INDTest> ToSortedINDTestVec(std::list<model::IND> const& inds) {
    std::vector<INDTest> ind_test_vec;
    ind_test_vec.reserve(inds.size());
    std::transform(inds.begin(), inds.end(), std::back_inserter(ind_test_vec), ToINDTest);
    std::sort(ind_test_vec.begin(), ind_test_vec.end());
    return ind_test_vec;
}

void CheckINDsListsEquality(std::list<model::IND> const& actual, INDTestSet const& expected) {
    ASSERT_EQ(actual.size(), expected.size())
            << "count of generated dependencies does not match: expected " << expected.size()
            << ", got " << actual.size();

    for (auto const& dep : actual) {
        if (expected.find(ToINDTest(dep)) == expected.end()) {
            FAIL() << "generated dep '" << dep.ToShortString() << "' is not present in expected";
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

void CheckINDsListsEqualityTest(std::unique_ptr<algos::INDAlgorithm> ind_algo,
                                INDTestSet const& expected_inds) {
    ind_algo->Execute();
    CheckINDsListsEquality(ind_algo->INDList(), expected_inds);
}

void CheckINDsResultContainsINDsTest(std::unique_ptr<algos::INDAlgorithm> ind_algo,
                                     INDTestSet const& expected_inds_subset,
                                     size_t expected_result_size) {
    ind_algo->Execute();
    ASSERT_EQ(ind_algo->INDList().size(), expected_result_size);
    CheckResultContainsINDs(ind_algo->INDList(), expected_inds_subset);
}

std::vector<INDEqualityTestConfig> const kINDEqualityTestConfigs{
        {{kIndTestWide2},
         INDTestSet{{{0, {2}}, {0, {0}}}, {{0, {3}}, {0, {1}}}, {{0, {2, 3}}, {0, {0, 1}}}}},
        {{kIndTestPlanets},
         INDTestSet{{{0, {0}}, {0, {1}}},
                    {{0, {1}}, {0, {0}}},
                    {{0, {2}}, {0, {3}}},
                    {{0, {3}}, {0, {2}}},
                    {{0, {1, 3}}, {0, {0, 2}}},
                    {{0, {0, 2}}, {0, {1, 3}}},
                    {{0, {0, 3}}, {0, {1, 2}}},
                    {{0, {1, 2}}, {0, {0, 3}}}}},
        {{kIndTest3aryInds},
         INDTestSet{{{0, {3}}, {0, {0}}},
                    {{0, {4}}, {0, {1}}},
                    {{0, {5}}, {0, {2}}},
                    {{0, {2}}, {0, {5}}},
                    {{0, {3, 4}}, {0, {0, 1}}},
                    {{0, {4, 5}}, {0, {1, 2}}},
                    {{0, {3, 5}}, {0, {0, 2}}},
                    {{0, {3, 4, 5}}, {0, {0, 1, 2}}},
                    {{0, {3, 4, 5}}, {0, {0, 1, 2}}}}},
};
}  // namespace tests
