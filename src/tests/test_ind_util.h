#pragma once
#include <list>
#include <string>
#include <utility>
#include <vector>
#include <set>

#include "algorithms/ind/ind.h"
#include "algorithms/ind/ind_algorithm.h"
#include "csv_config_util.h"

namespace tests {

using CCTest = std::pair<model::TableIndex, std::vector<model::ColumnIndex>>;
using INDTest = std::pair<CCTest, CCTest>;
using INDTestSet = std::set<INDTest>;

std::string TableNamesToString(CSVConfigs const& csv_configs);
INDTest ToINDTest(model::IND const& ind);
std::vector<INDTest> ToSortedINDTestVec(std::list<model::IND> const& inds);

void CheckINDsListsEquality(std::list<model::IND> const& actual, INDTestSet const& expected);
void CheckResultContainsINDs(std::list<model::IND> const& actual, INDTestSet expected_subset);

void CheckINDsListsEqualityTest(std::unique_ptr<algos::INDAlgorithm> ind_algo,
                                INDTestSet const& expected_inds);

void CheckINDsResultContainsINDsTest(std::unique_ptr<algos::INDAlgorithm> ind_algo,
                                     INDTestSet const& expected_inds_subset,
                                     size_t expected_result_size);

struct INDEqualityTestConfig {
    CSVConfigs csv_configs;
    INDTestSet expected_inds;
};

/*
 * Test configurations for which both Faida and other IND algorithms produce the same results.
 * If a config is found for which Faida produces a different number of dependencies (due to
 * the fact that Faida’s algorithm is approximate), then you will need to add this config to
 * a separate vector.
 */
extern std::vector<INDEqualityTestConfig> const kINDEqualityTestConfigs;

}  // namespace tests
