#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/parser/csv_parser/csv_parser.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {

namespace {

using ::testing::ContainerEq;

class TestCSVParser : public ::testing::Test {};

}  // namespace

static void CheckGetNextRow(CSVConfig const& table,
                            std::vector<std::vector<std::string>> const& expected) {
    config::InputTable parser = MakeInputTable(table);

    std::vector<std::vector<std::string>> actual;
    for (std::size_t index = 0; index < expected.size(); index++) {
        std::vector<std::string> row = parser->GetNextRow();
        actual.push_back(std::move(row));
    }

    ASSERT_THAT(actual, ContainerEq(expected)) << "Fail on " << table.path;
}

TEST(TestCSVParser, TestGetNextRow) {
    CheckGetNextRow(kNullEmpty,
                    {{"1", "NULL", "3", "1"}, {"1", "2", "", "1"}, {"1", "2", "3", "1"}});
    CheckGetNextRow(kTestSingleColumn, {{"1"}, {"2"}, {"3"}, {"3"}});
    CheckGetNextRow(kTestWide, {{"1", "3", "3", "4", "5"}, {"2", "3", "4", "4", "6"}});
    CheckGetNextRow(kTestEmpty, {});
    CheckGetNextRow(kAbalone,
                    {{"M", "0.455", "0.365", "0.095", "0.514", "0.2245", "0.101", "0.15", "15"}});
    CheckGetNextRow(kTestParse, {{"", "\\\\\\\"", "b\"b\\\\ b"},
                                 {"\"", "\\\\", "b\\"},
                                 {"a,bc", "a,\"bc", "a\",bc"},
                                 {"bb", "\\\\", "\\\\"},
                                 {"a", "a,a", "a"}});
}

static void CheckHasNextRow(CSVConfig const& table, std::size_t num_rows) {
    config::InputTable parser = MakeInputTable(table);
    if (table.has_header) num_rows--;

    for (std::size_t index = 0; index < num_rows; index++) {
        bool has_next = parser->HasNextRow();
        ASSERT_TRUE(has_next) << "Fail on " << table.path << ": expected true, actual false";
        parser->GetNextRow();
    }

    bool has_next = parser->HasNextRow();
    ASSERT_FALSE(has_next) << "Fail on " << table.path << ": expected false, actual true";
}

TEST(TestCSVParser, TestHasNextRow) {
    CheckHasNextRow(kACShippingDates, 6);
    CheckHasNextRow(kAdult, 32561);
    CheckHasNextRow(kTestEmpty, 1);
    CheckHasNextRow(kTest1, 20);
}

static void CheckReset(CSVConfig const& table, std::size_t num_rows) {
    config::InputTable parser = MakeInputTable(table);
    if (table.has_header) num_rows--;

    std::vector<std::vector<std::string>> first_parse;

    for (std::size_t index = 0; index < num_rows; index++) {
        std::vector<std::string> row = parser->GetNextRow();
        first_parse.push_back(std::move(row));
    }

    parser->Reset();

    std::vector<std::vector<std::string>> second_parse;
    for (std::size_t index = 0; index < num_rows; index++) {
        std::vector<std::string> row = parser->GetNextRow();
        second_parse.push_back(std::move(row));
    }

    ASSERT_THAT(first_parse, ContainerEq(second_parse)) << "Fail on " << table.path;
}

TEST(TestCSVParser, TestReset) {
    CheckReset(kACShippingDates, 6);
    CheckReset(kAdult, 32561);
    CheckReset(kTestEmpty, 1);
    CheckReset(kTest1, 20);
}

}  // namespace tests
