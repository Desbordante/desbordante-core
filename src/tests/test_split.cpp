#include <list>
#include <optional>
#include <set>
#include <string>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "all_csv_configs.h"
#include "config/names.h"
#include "csv_config_util.h"

namespace tests {

void CompareDDStringLists(std::set<std::pair<std::set<model::DFStringConstraint>,
                                             std::set<model::DFStringConstraint>>> const& expected,
                          std::list<model::DDString> const& actual) {
    ASSERT_EQ(expected.size(), actual.size())
            << "count of generated dependencies does not match: expected " << expected.size()
            << ", got " << actual.size();
    for (auto const& dd : actual) {
        std::set<model::DFStringConstraint> lhs(dd.left.begin(), dd.left.end());
        std::set<model::DFStringConstraint> rhs(dd.right.begin(), dd.right.end());
        if (expected.find(std::make_pair(std::move(lhs), std::move(rhs))) == expected.end()) {
            FAIL() << "generated dependency that is not expected";
        }
    }
}

class SplitAlgorithmTest : public ::testing::Test {
public:
    static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config,
                                           std::optional<CSVConfig> const& dif_table_csv_config) {
        using namespace config::names;
        if (dif_table_csv_config == std::nullopt) {
            return {{kCsvConfig, csv_config}};
        }
        return {{kCsvConfig, csv_config},
                {kDifferenceTable, MakeInputTable(dif_table_csv_config.value())}};
    }

    static std::unique_ptr<algos::dd::Split> CreateSplitAlgorithmInstance(
            CSVConfig const& csv_config,
            std::optional<CSVConfig> const& dif_table_csv_config = std::nullopt) {
        return algos::CreateAndLoadAlgorithm<algos::dd::Split>(
                GetParamMap(csv_config, dif_table_csv_config));
    }
};

TEST_F(SplitAlgorithmTest, Test0) {
    auto algo = CreateSplitAlgorithmInstance(kTestDD, kTestDif);
    algo->Execute();

    auto actual_results = algo->GetDDStringList();
    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            expected_results = {{{{"Col4", 2, 4}}, {{"Col0", 3, 4}}},
                                {{{"Col1", 2, 5}}, {{"Col0", 1, 1}}}};
    CompareDDStringLists(expected_results, actual_results);
}

TEST_F(SplitAlgorithmTest, Test1) {
    auto algo = CreateSplitAlgorithmInstance(kTestDD1);
    algo->Execute();

    auto actual_results = algo->GetDDStringList();
    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            expected_results = {{{{"Col1", 2, 3}}, {{"Col0", 1, 1}}},
                                {{{"Col0", 1, 1}}, {{"Col1", 2, 2}}}};
    CompareDDStringLists(expected_results, actual_results);
}

TEST_F(SplitAlgorithmTest, Test2) {
    auto algo = CreateSplitAlgorithmInstance(kTestDD2, kTestDif1);
    algo->Execute();

    auto actual_results = algo->GetDDStringList();
    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            expected_results = {{{{"Col3", 5, 5}}, {{"Col2", 4, 4}}}};
    CompareDDStringLists(expected_results, actual_results);
}

TEST_F(SplitAlgorithmTest, Test3) {
    auto algo = CreateSplitAlgorithmInstance(kTestDD2, kTestDif2);
    algo->Execute();

    auto actual_results = algo->GetDDStringList();

    // there are two possible minimal covers that differ in one deleted DD;
    // commented lines show which DD is deleted and which is present
    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            expected_results = {{{{"Col3", 7, 12}}, {{"Col1", 1, 1}}},
                                {{{"Col3", 5, 5}}, {{"Col1", 2, 2}}},
                                {{{"Col3", 5, 7}, {"Col2", 4, 4}}, {{"Col1", 2, 2}}},
                                //{{{"Col3", 5, 5}}, {{"Col2", 4, 4}}},
                                {{{"Col3", 12, 12}}, {{"Col2", 4, 4}}},
                                {{{"Col1", 2, 2}}, {{"Col2", 4, 4}}},
                                {{{"Col3", 7, 7}}, {{"Col2", 8, 8}}},
                                {{{"Col1", 1, 1}, {"Col3", 5, 7}}, {{"Col2", 8, 8}}},
                                {{{"Col1", 2, 2}}, {{"Col3", 5, 5}}},
                                {{{"Col2", 8, 8}}, {{"Col3", 7, 7}}},
                                {{{"Col1", 1, 1}}, {{"Col3", 7, 12}}},
                                {{{"Col1", 1, 1}, {"Col2", 4, 4}}, {{"Col3", 12, 12}}}};

    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            also_expected_results = {{{{"Col3", 7, 12}}, {{"Col1", 1, 1}}},
                                     {{{"Col3", 5, 5}}, {{"Col1", 2, 2}}},
                                     {{{"Col3", 5, 7}, {"Col2", 4, 4}}, {{"Col1", 2, 2}}},
                                     {{{"Col3", 5, 5}}, {{"Col2", 4, 4}}},
                                     {{{"Col3", 12, 12}}, {{"Col2", 4, 4}}},
                                     //{{{"Col1", 2, 2}}, {{"Col2", 4, 4}}},
                                     {{{"Col3", 7, 7}}, {{"Col2", 8, 8}}},
                                     {{{"Col1", 1, 1}, {"Col3", 5, 7}}, {{"Col2", 8, 8}}},
                                     {{{"Col1", 2, 2}}, {{"Col3", 5, 5}}},
                                     {{{"Col2", 8, 8}}, {{"Col3", 7, 7}}},
                                     {{{"Col1", 1, 1}}, {{"Col3", 7, 12}}},
                                     {{{"Col1", 1, 1}, {"Col2", 4, 4}}, {{"Col3", 12, 12}}}};

    CompareDDStringLists(expected_results, actual_results);
}

TEST_F(SplitAlgorithmTest, Test4) {
    auto algo = CreateSplitAlgorithmInstance(kTestDD3, kTestDif3);
    algo->Execute();

    auto actual_results = algo->GetDDStringList();

    // there are two possible minimal covers that differ in one deleted DD;
    // commented lines show which DD is deleted and which is present
    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            expected_results = {{{{"Col3", 7, 7}}, {{"Col2", 4, 4}}},
                                {{{"Col1", 2, 2}}, {{"Col3", 7, 7}}},
                                //{{{"Col1", 2, 2}}, {{"Col2", 4, 4}}},
                                {{{"Col2", 4, 4}}, {{"Col3", 7, 7}}}};

    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            also_expected_results = {{{{"Col3", 7, 7}}, {{"Col2", 4, 4}}},
                                     //{{{"Col1", 2, 2}}, {{"Col3", 7, 7}}},
                                     {{{"Col1", 2, 2}}, {{"Col2", 4, 4}}},
                                     {{{"Col2", 4, 4}}, {{"Col3", 7, 7}}}};
    CompareDDStringLists(expected_results, actual_results);
}

TEST_F(SplitAlgorithmTest, Test5) {
    auto algo = CreateSplitAlgorithmInstance(kTestDD4, kTestDif4);
    algo->Execute();

    auto actual_results = algo->GetDDStringList();

    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            expected_results = {{{{"Col2", 4, 8}}, {{"Col1", 1, 2}}},
                                {{{"Col3", 5, 5}}, {{"Col1", 2, 2}}},
                                {{{"Col2", 4, 4}, {"Col3", 5, 7}}, {{"Col1", 2, 2}}},
                                {{{"Col2", 8, 8}}, {{"Col1", 1, 1}}},
                                {{{"Col1", 2, 2}}, {{"Col2", 4, 8}}},
                                {{{"Col1", 1, 2}, {"Col3", 5, 7}}, {{"Col2", 4, 8}}},
                                {{{"Col1", 1, 1}, {"Col3", 5, 7}}, {{"Col2", 8, 8}}},
                                {{{"Col3", 5, 5}}, {{"Col2", 4, 4}}},
                                {{{"Col1", 2, 2}, {"Col3", 5, 12}}, {{"Col2", 4, 4}}},
                                {{{"Col1", 1, 1}}, {{"Col3", 5, 12}}},
                                {{{"Col2", 4, 4}}, {{"Col3", 5, 12}}},
                                {{{"Col2", 8, 8}}, {{"Col3", 5, 7}}},
                                {{{"Col1", 2, 2}, {"Col2", 4, 4}}, {{"Col3", 5, 5}}}};

    CompareDDStringLists(expected_results, actual_results);
}

}  // namespace tests
