#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include "config/tabular_data/input_table_type.h"
#include "parser/csv_parser/csv_parser.h"

namespace tests {

static auto const test_data_dir = std::filesystem::current_path() / "input_data";

/// csv table configuration info to create an input table
struct TableConfig {
    std::string_view name;
    char separator;
    bool has_header;

    std::filesystem::path GetPath() const {
        return test_data_dir / name;
    }

    config::InputTable MakeInputTable() const {
        return std::make_shared<CSVParser>(GetPath(), separator, has_header);
    }
};

/// a struct consisting of a table config and the expected hash
struct TableConfigHash {
    TableConfig config;
    size_t hash;
};

/// a struct consisting of a tables config and the expected hash
struct TablesConfigHash {
    std::vector<TableConfig> configs;
    size_t hash;
};

}  // namespace tests
