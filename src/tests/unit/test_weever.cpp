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
#include "tests/mock/mock_input_table.h"

namespace tests {

using namespace algos;
using namespace config::names;

// DC: no two same-state tuples where lower salary has higher tax rate.
static std::string const kSalaryDC =
        "!(s.State == t.State and s.Salary <= t.Salary and s.FedTaxRate >= t.FedTaxRate)";

static std::vector<std::string> const kSalaryColumns = {"State", "Salary", "FedTaxRate"};

static void ApplyAndExecute(Weever& weever, config::InputTable inserts = {},
                            std::unordered_set<size_t> deletes = {},
                            config::InputTable updates = {}) {
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
    weever->Execute();
    EXPECT_TRUE(!weever->GetViolations().empty());

    ApplyAndExecute(*weever, {}, {5, 10, 15});
    std::vector<dc::Violation> expected = {{8, 6}, {8, 7}, {8, 9}, {14, 13}};
    auto actual = weever->GetViolations();
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(actual));

    ApplyAndExecute(*weever, {}, {8, 13});
    EXPECT_TRUE(weever->GetViolations().empty());
}

TEST(Weever, DeleteInsert) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    weever->Execute();
    EXPECT_TRUE(!weever->GetViolations().empty());

    ApplyAndExecute(*weever, {}, {11});
    EXPECT_TRUE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const kWeeverInsertRows = {{"Texas", "3000", "0.31"}};
    config::InputTable input_table =
            std::make_shared<MockTable>("insert", kSalaryColumns, kWeeverInsertRows);
    ApplyAndExecute(*weever, input_table, {});
    std::vector<dc::Violation> expected = {{12, 10}};
    for (auto const& [f, s] : weever->GetViolations()) {
        LOG_INFO("({}, {})", f, s);
    }
    auto actual = weever->GetViolations();
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(actual));
}

TEST(Weever, Update) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    weever->Execute();
    std::vector<dc::Violation> expected = {{11, 10}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));

    std::vector<model::IDatasetStream::Row> const kWeeverUpdateRows = {
            {"11", "Texas", "3100", "0.31"}};
    auto update_table = std::make_shared<MockTable>("update", kSalaryColumns, kWeeverUpdateRows);
    ApplyAndExecute(*weever, {}, {}, update_table);

    EXPECT_TRUE(weever->GetViolations().empty());
}

TEST(Weever, UpdateMultiple) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC5},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    weever->Execute();
    EXPECT_FALSE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const kWeeverUpdateRows = {
            {"5", "NewYork", "6000", "0.4"},
            {"8", "Wisconsin", "7000", "0.3"},
            {"10", "Texas", "5000", "0.5"},
            {"14", "Texas", "3100", "0.31"},
            {"15", "Texas", "4000", "0.4"}};
    config::InputTable update_table =
            std::make_shared<MockTable>("update", kSalaryColumns, kWeeverUpdateRows);
    ApplyAndExecute(*weever, {}, {}, update_table);
    for (auto const& [f, s] : weever->GetViolations()) {
        LOG_INFO("({}, {})", f, s);
    }
    EXPECT_TRUE(weever->GetViolations().empty());
}

TEST(Weever, IncrementalInsertsAllViolate) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    weever->Execute();
    EXPECT_TRUE(!weever->GetViolations().empty());

    // Delete all 10 data rows from TestDC1 (IDs 2–11, header is line 1)
    ApplyAndExecute(*weever, {}, {2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
    EXPECT_TRUE(weever->GetViolations().empty());

    // Convert k (1..1000) to a 3-decimal fixed-point string: 1→"0.001", 1000→"1.000"
    auto FormatTax = [](size_t k) -> std::string {
        std::string s = std::to_string(k);
        while (s.size() < 4) s = "0" + s;
        return s.substr(0, s.size() - 3) + "." + s.substr(s.size() - 3);
    };

    static constexpr size_t kFirstId = 12;
    static constexpr size_t kNumTuples = 1000;

    std::vector<model::IDatasetStream::Row> rows;
    rows.reserve(kNumTuples);
    for (size_t k = 1; k <= kNumTuples; ++k) {
        rows.push_back({"Texas", std::to_string(kNumTuples - k), FormatTax(k)});
    }
    auto insertTable = std::make_shared<MockTable>("insert", kSalaryColumns, rows);
    ApplyAndExecute(*weever, insertTable);

    std::vector<dc::Violation> expected;
    expected.reserve(kNumTuples * (kNumTuples - 1) / 2);
    for (size_t j = kFirstId + 1; j < kFirstId + kNumTuples; ++j) {
        for (size_t i = kFirstId; i < j; ++i) {
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
    weever->Execute();
    std::vector<dc::Violation> initial = {{11, 10}};
    ASSERT_THAT(initial, testing::UnorderedElementsAreArray(weever->GetViolations()));

    // (s=10,t=9): 3000<=3001, 0.30>=0.29 → YES; (s=11,t=9): 3000<=3001, 0.31>=0.29 → YES
    std::vector<model::IDatasetStream::Row> const kUpdateRows = {{"9", "Texas", "3001", "0.29"}};
    auto updateTable = std::make_shared<MockTable>("update", kSalaryColumns, kUpdateRows);
    ApplyAndExecute(*weever, {}, {}, updateTable);

    std::vector<dc::Violation> expected = {{10, 9}, {11, 9}, {11, 10}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

TEST(Weever, InsertNoViolation) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    weever->Execute();
    EXPECT_FALSE(weever->GetViolations().empty());

    ApplyAndExecute(*weever, {}, {10, 11});
    EXPECT_TRUE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const kInsertRows = {{"Texas", "500", "0.10"}};
    auto insertTable = std::make_shared<MockTable>("insert", kSalaryColumns, kInsertRows);
    ApplyAndExecute(*weever, insertTable);
    EXPECT_TRUE(weever->GetViolations().empty());
}

// Calling Execute with no insert/delete/update operations must leave the
// violation set unchanged across repeated invocations.
TEST(Weever, IdempotentExecute) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    weever->Execute();
    std::vector<dc::Violation> expected = {{11, 10}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));

    ApplyAndExecute(*weever);
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));

    ApplyAndExecute(*weever);
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

// Deleting every row in the table must leave no violations.
// TestDC5 has 14 data rows at IDs 2–15.
TEST(Weever, DeleteAllRows) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC5},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    weever->Execute();
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
    weever->Execute();
    std::vector<dc::Violation> initial = {{11, 10}};
    ASSERT_THAT(initial, testing::UnorderedElementsAreArray(weever->GetViolations()));

    std::vector<model::IDatasetStream::Row> const kInsertRows = {{"Texas", "3000", "0.50"}};
    auto insertTable = std::make_shared<MockTable>("insert", kSalaryColumns, kInsertRows);
    std::vector<model::IDatasetStream::Row> const kUpdateRows = {{"9", "Texas", "2100", "0.26"}};
    auto updateTable = std::make_shared<MockTable>("update", kSalaryColumns, kUpdateRows);
    ApplyAndExecute(*weever, insertTable, {11}, updateTable);

    std::vector<dc::Violation> expected = {{12, 10}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

TEST(Weever, DeleteThenReinsert) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    weever->Execute();
    EXPECT_FALSE(weever->GetViolations().empty());

    ApplyAndExecute(*weever, {}, {10, 11});
    EXPECT_TRUE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const kInsertRows = {{"Texas", "3000", "0.30"},
                                                                 {"Texas", "3000", "0.31"}};
    auto insertTable = std::make_shared<MockTable>("insert", kSalaryColumns, kInsertRows);
    ApplyAndExecute(*weever, insertTable);

    std::vector<dc::Violation> expected = {{13, 12}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

TEST(Weever, SequentialUpdates) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    weever->Execute();
    std::vector<dc::Violation> initial = {{11, 10}};
    ASSERT_THAT(initial, testing::UnorderedElementsAreArray(weever->GetViolations()));

    // Row 11 → (Texas, 3200, 0.35): strictly higher salary and tax than row 10,
    // so neither ordering of the pair satisfies the predicate.
    std::vector<model::IDatasetStream::Row> const kUpdateResolve = {
            {"11", "Texas", "3200", "0.35"}};
    auto resolveTable = std::make_shared<MockTable>("update", kSalaryColumns, kUpdateResolve);
    ApplyAndExecute(*weever, {}, {}, resolveTable);
    EXPECT_TRUE(weever->GetViolations().empty());

    // Revert row 11 to original values — (s=11,t=10): 3000<=3000, 0.31>=0.30 → YES.
    std::vector<model::IDatasetStream::Row> const kUpdateRevert = {{"11", "Texas", "3000", "0.31"}};
    auto revertTable = std::make_shared<MockTable>("update", kSalaryColumns, kUpdateRevert);
    ApplyAndExecute(*weever, {}, {}, revertTable);
    std::vector<dc::Violation> expected = {{11, 10}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

TEST(Weever, InsertIntoEmptyState) {
    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kSalaryDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    weever->Execute();

    ApplyAndExecute(*weever, {}, {2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
    EXPECT_TRUE(weever->GetViolations().empty());

    std::vector<model::IDatasetStream::Row> const kInsertRows = {
            {"Texas", "3000", "0.30"},
            {"Texas", "2000", "0.40"},
            {"Texas", "1000", "0.50"},
    };
    auto insertTable = std::make_shared<MockTable>("insert", kSalaryColumns, kInsertRows);
    ApplyAndExecute(*weever, insertTable);

    std::vector<dc::Violation> expected = {{13, 12}, {14, 12}, {14, 13}};
    ASSERT_THAT(expected, testing::UnorderedElementsAreArray(weever->GetViolations()));
}

}  // namespace tests
