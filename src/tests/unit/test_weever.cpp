#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/dc/weever/weever.h"
#include "core/config/names.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/idataset_stream.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"
#include "tests/mock/mock_dataset_stream.h"

namespace tests {

using namespace algos;
using namespace config::names;

// DC: no two same-state tuples where lower salary has higher tax rate.
static std::string const kSalaryDC =
        "!(s.State == t.State and s.Salary <= t.Salary and s.FedTaxRate >= t.FedTaxRate)";

static std::vector<std::string> const kSalaryColumns = {"State", "Salary", "FedTaxRate"};

static void ApplyAndExecute(Weever& weever, config::InputTable inserts = {},
                            std::unordered_set<size_t> deletes = {},
                            config::InputTable updates = {}, std::string const& dc = kSalaryDC) {
    weever.SetOption(kDenialConstraint, dc);
    weever.SetOption(kInsertStatements, std::move(inserts));
    weever.SetOption(kDeleteStatements, std::move(deletes));
    weever.SetOption(kUpdateStatements, std::move(updates));
    weever.Execute();
}

TEST(Weever, Delete) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC5},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    EXPECT_TRUE(!weever->GetViolations().empty());

    ApplyAndExecute(*weever, {}, {5, 10, 15});
    std::vector<dc::Violation> expected = {{8, 6}, {8, 7}, {8, 9}, {14, 13}};
    auto actual = weever->GetViolations();
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(actual));

    ApplyAndExecute(*weever, {}, {8, 13});
    EXPECT_TRUE(weever->GetViolations().empty());
}

TEST(Weever, Insert) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    ApplyAndExecute(*weever, {}, {11});

    EXPECT_TRUE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const insert_rows = {{"Texas", "5000", "0.05"}};
    config::InputTable input_table =
            std::make_shared<MockDatasetStream>("insert", kSalaryColumns, insert_rows);
    ApplyAndExecute(*weever, input_table, {});
    std::vector<dc::Violation> expected = {{10, 12}, {9, 12}, {8, 12}};
    auto actual = weever->GetViolations();
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(actual));
}

TEST(Weever, DeleteInsert) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    EXPECT_TRUE(!weever->GetViolations().empty());

    ApplyAndExecute(*weever, {}, {11});
    EXPECT_TRUE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const insert_rows = {{"Texas", "3000", "0.31"}};
    config::InputTable input_table =
            std::make_shared<MockDatasetStream>("insert", kSalaryColumns, insert_rows);
    ApplyAndExecute(*weever, input_table, {});
    std::vector<dc::Violation> expected = {{12, 10}};
    auto actual = weever->GetViolations();
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(actual));
}

TEST(Weever, Update) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    std::vector<dc::Violation> expected = {{11, 10}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));

    std::vector<model::IDatasetStream::Row> const update_rows = {{"11", "Texas", "3100", "0.31"}};
    auto update_table = std::make_shared<MockDatasetStream>("update", kSalaryColumns, update_rows);
    ApplyAndExecute(*weever, {}, {}, update_table);

    EXPECT_TRUE(weever->GetViolations().empty());
}

TEST(Weever, UpdateMultiple) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC5},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    EXPECT_FALSE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const update_rows = {{"5", "NewYork", "6000", "0.4"},
                                                                 {"8", "Wisconsin", "7000", "0.3"},
                                                                 {"10", "Texas", "5000", "0.5"},
                                                                 {"14", "Texas", "3100", "0.31"},
                                                                 {"15", "Texas", "4000", "0.4"}};
    config::InputTable update_table =
            std::make_shared<MockDatasetStream>("update", kSalaryColumns, update_rows);
    ApplyAndExecute(*weever, {}, {}, update_table);
    EXPECT_TRUE(weever->GetViolations().empty());
}

TEST(Weever, InsertEqual) {
    std::string const dc = "!(s.State == t.State and s.Salary == t.Salary)";
    algos::StdParamsMap params = {{kCsvConfig, kTestDC4}, {kDenialConstraint, dc}};
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever, {}, {}, {}, dc);
    EXPECT_TRUE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const insert_rows = {{"NewYork", "3000", "0.4"},
                                                                 {"Texas", "5000", "0.1"}};
    config::InputTable insert_table =
            std::make_shared<MockDatasetStream>("insert", kSalaryColumns, insert_rows);
    ApplyAndExecute(*weever, insert_table, {}, {}, dc);

    std::vector<dc::Violation> expected = {{2, 13}, {13, 2}, {14, 9}, {9, 14}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

TEST(Weever, IncrementalInsertsAllViolate) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    EXPECT_TRUE(!weever->GetViolations().empty());

    // Delete all rows
    ApplyAndExecute(*weever, {}, {2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
    EXPECT_TRUE(weever->GetViolations().empty());

    // Convert k (1..1000) to a 3-decimal fixed-point string: 1→"0.001", 1000→"1.000"
    auto format_tax = [](size_t k) -> std::string {
        std::string s = std::to_string(k);
        while (s.size() < 4) s = "0" + s;
        return s.substr(0, s.size() - 3) + "." + s.substr(s.size() - 3);
    };

    size_t first_id = 12;
    size_t num_tuples = 1000;

    std::vector<model::IDatasetStream::Row> rows;
    rows.reserve(num_tuples);
    for (size_t k = 1; k <= num_tuples; ++k) {
        rows.push_back({"Texas", std::to_string(num_tuples - k), format_tax(k)});
    }
    auto insert_table = std::make_shared<MockDatasetStream>("insert", kSalaryColumns, rows);
    ApplyAndExecute(*weever, insert_table);

    std::vector<dc::Violation> expected;
    expected.reserve(num_tuples * (num_tuples - 1) / 2);
    for (size_t j = first_id + 1; j < first_id + num_tuples; ++j) {
        for (size_t i = first_id; i < j; ++i) {
            expected.push_back({j, i});
        }
    }

    auto actual = weever->GetViolations();
    std::sort(actual.begin(), actual.end());
    ASSERT_THAT(expected, actual);
}

TEST(Weever, UpdateCreatesViolation) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    std::vector<dc::Violation> initial = {{11, 10}};
    ASSERT_THAT(initial, testing::UnorderedElementsAreArray(weever->GetViolations()));

    std::vector<model::IDatasetStream::Row> const update_rows = {{"9", "Texas", "3001", "0.29"}};
    auto update_table = std::make_shared<MockDatasetStream>("update", kSalaryColumns, update_rows);
    ApplyAndExecute(*weever, {}, {}, update_table);

    std::vector<dc::Violation> expected = {{10, 9}, {11, 9}, {11, 10}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

TEST(Weever, InsertNoViolation) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    EXPECT_FALSE(weever->GetViolations().empty());

    ApplyAndExecute(*weever, {}, {10, 11});
    EXPECT_TRUE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const insert_rows = {{"Texas", "500", "0.10"}};
    auto insert_table = std::make_shared<MockDatasetStream>("insert", kSalaryColumns, insert_rows);
    ApplyAndExecute(*weever, insert_table);
    EXPECT_TRUE(weever->GetViolations().empty());
}

TEST(Weever, IdempotentExecute) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    std::vector<dc::Violation> expected = {{11, 10}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));

    ApplyAndExecute(*weever);
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));

    ApplyAndExecute(*weever);
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

TEST(Weever, DeleteAllRows) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC5},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    EXPECT_FALSE(weever->GetViolations().empty());

    ApplyAndExecute(*weever, {}, {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});
    EXPECT_TRUE(weever->GetViolations().empty());
}

TEST(Weever, CombinedOps) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    std::vector<dc::Violation> initial = {{11, 10}};
    ASSERT_THAT(initial, testing::UnorderedElementsAreArray(weever->GetViolations()));

    std::vector<model::IDatasetStream::Row> const insert_rows = {{"Texas", "3000", "0.50"}};
    auto insert_table = std::make_shared<MockDatasetStream>("insert", kSalaryColumns, insert_rows);
    std::vector<model::IDatasetStream::Row> const update_rows = {{"9", "Texas", "2100", "0.26"}};
    auto update_table = std::make_shared<MockDatasetStream>("update", kSalaryColumns, update_rows);
    ApplyAndExecute(*weever, insert_table, {11}, update_table);

    std::vector<dc::Violation> expected = {{12, 10}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

TEST(Weever, DeleteThenReinsert) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    EXPECT_FALSE(weever->GetViolations().empty());

    ApplyAndExecute(*weever, {}, {10, 11});
    EXPECT_TRUE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const insert_rows = {{"Texas", "3000", "0.30"},
                                                                 {"Texas", "3000", "0.31"}};
    auto insert_table = std::make_shared<MockDatasetStream>("insert", kSalaryColumns, insert_rows);
    ApplyAndExecute(*weever, insert_table);

    std::vector<dc::Violation> expected = {{13, 12}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

TEST(Weever, SequentialUpdates) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);
    std::vector<dc::Violation> initial = {{11, 10}};
    ASSERT_THAT(initial, testing::UnorderedElementsAreArray(weever->GetViolations()));

    std::vector<model::IDatasetStream::Row> const update_resolve = {
            {"11", "Texas", "3200", "0.35"}};
    auto resolve_table =
            std::make_shared<MockDatasetStream>("update", kSalaryColumns, update_resolve);
    ApplyAndExecute(*weever, {}, {}, resolve_table);
    EXPECT_TRUE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const update_revert = {{"11", "Texas", "3000", "0.31"}};
    auto revert_table =
            std::make_shared<MockDatasetStream>("update", kSalaryColumns, update_revert);
    ApplyAndExecute(*weever, {}, {}, revert_table);
    std::vector<dc::Violation> expected = {{11, 10}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

TEST(Weever, InsertIntoEmptyState) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    ApplyAndExecute(*weever);

    ApplyAndExecute(*weever, {}, {2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
    EXPECT_TRUE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const insert_rows = {
            {"Texas", "3000", "0.30"},
            {"Texas", "2000", "0.40"},
            {"Texas", "1000", "0.50"},
    };
    auto insert_table = std::make_shared<MockDatasetStream>("insert", kSalaryColumns, insert_rows);
    ApplyAndExecute(*weever, insert_table);

    std::vector<dc::Violation> expected = {{13, 12}, {14, 12}, {14, 13}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

}  // namespace tests
