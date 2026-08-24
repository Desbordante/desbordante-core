#include <list>
#include <optional>
#include <set>
#include <string>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/fastdd/fastdd.h"
#include "core/algorithms/dd/split/split.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

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

template <typename AlgorithmUnderTest>
class DDAlgorithmTest : public ::testing::Test {
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

    static std::unique_ptr<algos::dd::DDAlgorithm> CreateDDAlgorithmInstance(
            CSVConfig const& csv_config,
            std::optional<CSVConfig> const& dif_table_csv_config = std::nullopt) {
        return algos::CreateAndLoadAlgorithm<AlgorithmUnderTest>(
                GetParamMap(csv_config, dif_table_csv_config));
    }
};

TYPED_TEST_SUITE_P(DDAlgorithmTest);

TYPED_TEST_P(DDAlgorithmTest, Test0) {
    auto algo = this->CreateDDAlgorithmInstance(kTestDD, kTestDif);
    algo->Execute();

    auto actual_results = algo->DDList();
    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            expected_results = {{{{"Col4", 2, 4}}, {{"Col0", 3, 4}}},
                                {{{"Col1", 2, 5}}, {{"Col0", 1, 1}}}};
    CompareDDStringLists(expected_results, actual_results);
}

TYPED_TEST_P(DDAlgorithmTest, Test1) {
    auto algo = this->CreateDDAlgorithmInstance(kTestDD1, kTestDif1);
    algo->Execute();

    auto actual_results = algo->DDList();
    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            expected_results = {{{{"Col1", 2, 3}}, {{"Col0", 1, 1}}},
                                {{{"Col0", 1, 1}}, {{"Col1", 2, 2}}}};
    CompareDDStringLists(expected_results, actual_results);
}

TYPED_TEST_P(DDAlgorithmTest, Test2) {
    auto algo = this->CreateDDAlgorithmInstance(kTestDD2, kTestDif2);
    algo->Execute();

    auto actual_results = algo->DDList();
    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            expected_results = {{{{"Col3", 5, 5}}, {{"Col2", 4, 4}}}};
    CompareDDStringLists(expected_results, actual_results);
}

TYPED_TEST_P(DDAlgorithmTest, Test3) {
    auto algo = this->CreateDDAlgorithmInstance(kTestDD2, kTestDif3);
    algo->Execute();

    auto actual_results = algo->DDList();

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

    CompareDDStringLists(also_expected_results, actual_results);
}

TYPED_TEST_P(DDAlgorithmTest, Test4) {
    auto algo = this->CreateDDAlgorithmInstance(kTestDD3, kTestDif4);
    algo->Execute();

    auto actual_results = algo->DDList();

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

TYPED_TEST_P(DDAlgorithmTest, Test5) {
    auto algo = this->CreateDDAlgorithmInstance(kTestDD4, kTestDif5);
    algo->Execute();

    auto actual_results = algo->DDList();

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

TYPED_TEST_P(DDAlgorithmTest, TestDoubleCompare) {
    auto algo = this->CreateDDAlgorithmInstance(kTestDD5, kTestDif6);
    algo->Execute();

    auto actual_results = algo->DDList();
    std::set<std::pair<std::set<model::DFStringConstraint>, std::set<model::DFStringConstraint>>>
            expected_results = {{{{"Col3", 5.1, 5.1}}, {{"Col2", 4, 4}}}};
    CompareDDStringLists(expected_results, actual_results);
}

REGISTER_TYPED_TEST_SUITE_P(DDAlgorithmTest, Test0, Test1, Test2, Test3, Test4, Test5,
                            TestDoubleCompare);

using Algorithms = ::testing::Types<algos::dd::Split, algos::dd::FastDD>;
INSTANTIATE_TYPED_TEST_SUITE_P(DDAlgorithmTest, DDAlgorithmTest, Algorithms);

}  // namespace tests
