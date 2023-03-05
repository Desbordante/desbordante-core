#include <filesystem>
#include <vector>

#include <gtest/gtest.h>

#include "algo_factory.h"
#include "algorithms/spider/spider.h"
#include "datasets.h"
#include "model/ind.h"

namespace tests {

using namespace algos::ind;

/* simplified types for expected test data */
using IndCCTest = std::pair<unsigned, std::vector<unsigned>>;
using IndsTest = std::set<std::pair<IndCCTest, IndCCTest>>;
using DatasetsOrderTest = std::vector<std::string>;

void CheckINDsListsEquality(algos::PINDAlgorithm::INDList const& actual, IndsTest const& expected) {
    ASSERT_EQ(actual.size(), expected.size())
            << "count of generated dependencies does not match: expected " << expected.size()
            << ", got " << actual.size();

    auto to_test_cc = [](model::IND::ColumnCombination const& ind) -> IndCCTest {
        return std::make_pair(ind.table_index, ind.column_indices);
    };

    for (auto const& rule : actual) {
        if (expected.find(std::make_pair(to_test_cc(rule.GetLhs()), to_test_cc(rule.GetRhs()))) ==
            expected.end()) {
            FAIL() << "generated deps '" << rule.ToString() << "' is not present in expected";
        }
    }
    SUCCEED();
}

void CheckTablesInfoEquality(algos::PINDAlgorithm::DatasetsOrder const& actual,
                             DatasetsOrderTest const& expected) {
    ASSERT_EQ(actual.size(), expected.size()) << "count of datasets does not match: expected "
                                              << expected.size() << ", got " << actual.size();

    for (unsigned table_id = 0; table_id != expected.size(); ++table_id) {
        if (expected[table_id] != actual[table_id].table_name) {
            FAIL() << "Expected table name '" << expected[table_id] << "', but received '"
                   << actual[table_id].table_name << "'";
        }
    }
    SUCCEED();
}

class INDAlgorithmTest : public ::testing::Test {
protected:
    static algos::StdParamsMap GetParamMap(ColType col_type, KeyType key_type,
                                           std::string const& filename, char separator = ',',
                                           bool has_header = true) {
        using namespace algos::config::names;
        return {{kData, test_data_dir / filename},
                {kSeparator, separator},
                {kHasHeader, has_header},
                {kColType, col_type},
                {kKeyType, key_type}};
    }

    template <typename Algo, typename... Args>
    static decltype(auto) CreateAlgorithmInstance(Args&&... args) {
        return algos::CreateAndLoadPrimitive<Algo>(GetParamMap(std::forward<Args>(args)...));
    }

    template <typename... Args>
    static void TestAllSpiderImpls(IndsTest expected_inds, DatasetsOrderTest expected_order,
                                   Args&&... args) {
        for (ColType col_type : ColType::_values()) {
            for (KeyType key_type : KeyType::_values()) {
                auto algo = CreateAlgorithmInstance<algos::Spider>(col_type, key_type,
                                                                   std::forward<Args>(args)...);
                algo->Execute();

                CheckINDsListsEquality(algo->IndList(), expected_inds);
                CheckTablesInfoEquality(algo->GetDatasetsOrder(), expected_order);
            }
        }
    }
};

TEST_F(INDAlgorithmTest, Abalone) {
    IndsTest expected_inds{{{0, {2}}, {0, {5}}}};
    std::string filename = "abalone.csv";

    DatasetsOrderTest expected_order{filename};
    TestAllSpiderImpls(expected_inds, expected_order, filename, ',', false);
}

TEST_F(INDAlgorithmTest, CIPublic700) {
    IndsTest expected_inds{{{{0, {17}}, {0, {0}}},  {{0, {17}}, {0, {1}}},  {{0, {17}}, {0, {2}}},
                            {{0, {17}}, {0, {3}}},  {{0, {17}}, {0, {4}}},  {{0, {17}}, {0, {5}}},
                            {{0, {17}}, {0, {6}}},  {{0, {17}}, {0, {7}}},  {{0, {17}}, {0, {8}}},
                            {{0, {17}}, {0, {9}}},  {{0, {17}}, {0, {10}}}, {{0, {17}}, {0, {11}}},
                            {{0, {17}}, {0, {12}}}, {{0, {17}}, {0, {13}}}, {{0, {17}}, {0, {14}}},
                            {{0, {17}}, {0, {15}}}, {{0, {17}}, {0, {16}}}, {{0, {16}}, {0, {0}}},
                            {{0, {16}}, {0, {1}}},  {{0, {16}}, {0, {2}}},  {{0, {16}}, {0, {3}}},
                            {{0, {16}}, {0, {4}}},  {{0, {16}}, {0, {5}}},  {{0, {16}}, {0, {6}}},
                            {{0, {16}}, {0, {7}}},  {{0, {16}}, {0, {8}}},  {{0, {16}}, {0, {9}}},
                            {{0, {16}}, {0, {10}}}, {{0, {16}}, {0, {11}}}, {{0, {16}}, {0, {12}}},
                            {{0, {16}}, {0, {13}}}, {{0, {16}}, {0, {14}}}, {{0, {16}}, {0, {15}}},
                            {{0, {16}}, {0, {17}}}, {{0, {15}}, {0, {4}}},  {{0, {14}}, {0, {3}}},
                            {{0, {14}}, {0, {6}}},  {{0, {10}}, {0, {0}}},  {{0, {10}}, {0, {1}}},
                            {{0, {10}}, {0, {2}}},  {{0, {10}}, {0, {3}}},  {{0, {10}}, {0, {4}}},
                            {{0, {10}}, {0, {5}}},  {{0, {10}}, {0, {6}}},  {{0, {10}}, {0, {7}}},
                            {{0, {10}}, {0, {8}}},  {{0, {10}}, {0, {9}}},  {{0, {10}}, {0, {11}}},
                            {{0, {10}}, {0, {12}}}, {{0, {10}}, {0, {13}}}, {{0, {10}}, {0, {14}}},
                            {{0, {10}}, {0, {15}}}, {{0, {10}}, {0, {16}}}, {{0, {10}}, {0, {17}}},
                            {{0, {9}}, {0, {0}}},   {{0, {9}}, {0, {1}}},   {{0, {9}}, {0, {2}}},
                            {{0, {9}}, {0, {3}}},   {{0, {9}}, {0, {4}}},   {{0, {9}}, {0, {5}}},
                            {{0, {9}}, {0, {6}}},   {{0, {9}}, {0, {7}}},   {{0, {9}}, {0, {8}}},
                            {{0, {9}}, {0, {10}}},  {{0, {9}}, {0, {11}}},  {{0, {9}}, {0, {12}}},
                            {{0, {9}}, {0, {13}}},  {{0, {9}}, {0, {14}}},  {{0, {9}}, {0, {15}}},
                            {{0, {9}}, {0, {16}}},  {{0, {9}}, {0, {17}}},  {{0, {8}}, {0, {0}}},
                            {{0, {8}}, {0, {1}}},   {{0, {8}}, {0, {2}}},   {{0, {8}}, {0, {3}}},
                            {{0, {8}}, {0, {4}}},   {{0, {8}}, {0, {5}}},   {{0, {8}}, {0, {6}}},
                            {{0, {8}}, {0, {7}}},   {{0, {8}}, {0, {9}}},   {{0, {8}}, {0, {10}}},
                            {{0, {8}}, {0, {11}}},  {{0, {8}}, {0, {12}}},  {{0, {8}}, {0, {13}}},
                            {{0, {8}}, {0, {14}}},  {{0, {8}}, {0, {15}}},  {{0, {8}}, {0, {16}}},
                            {{0, {8}}, {0, {17}}},  {{0, {5}}, {0, {13}}}}};

    std::string filename = "CIPublicHighway700.csv";
    DatasetsOrderTest expected_order{filename};

    TestAllSpiderImpls(expected_inds, expected_order, filename);
}

TEST_F(INDAlgorithmTest, EpicMeds) {
    IndsTest expected_inds{{{{0, {8}}, {0, {0}}},
                            {{0, {8}}, {0, {1}}},
                            {{0, {8}}, {0, {2}}},
                            {{0, {8}}, {0, {7}}},
                            {{0, {7}}, {0, {0}}},
                            {{0, {7}}, {0, {1}}},
                            {{0, {7}}, {0, {2}}},
                            {{0, {7}}, {0, {8}}},
                            {{0, {6}}, {0, {3}}},
                            {{0, {6}}, {0, {4}}},
                            {{0, {5}}, {0, {3}}},
                            {{0, {5}}, {0, {4}}},
                            {{0, {5}}, {0, {6}}},
                            {{0, {3}}, {0, {4}}},
                            {{0, {2}}, {0, {0}}},
                            {{0, {2}}, {0, {1}}},
                            {{0, {0}}, {0, {1}}}}};
    std::string filename = "EpicMeds.csv";
    DatasetsOrderTest expected_order{filename};

    TestAllSpiderImpls(expected_inds, expected_order, filename, '|', true);
}

TEST_F(INDAlgorithmTest, EpicVitals) {
    IndsTest expected_inds{{{{0, {5}}, {0, {0}}},
                            {{0, {5}}, {0, {1}}},
                            {{0, {5}}, {0, {2}}},
                            {{0, {4}}, {0, {0}}},
                            {{0, {4}}, {0, {1}}},
                            {{0, {4}}, {0, {2}}},
                            {{0, {4}}, {0, {5}}},
                            {{0, {3}}, {0, {0}}},
                            {{0, {3}}, {0, {1}}},
                            {{0, {3}}, {0, {2}}},
                            {{0, {3}}, {0, {4}}},
                            {{0, {3}}, {0, {5}}},
                            {{0, {2}}, {0, {1}}},
                            {{0, {0}}, {0, {1}}},
                            {{0, {0}}, {0, {2}}}}};
    std::string filename = "EpicVitals.csv";
    DatasetsOrderTest expected_order{filename};

    TestAllSpiderImpls(expected_inds, expected_order, filename, '|', true);
}

}  // namespace tests
