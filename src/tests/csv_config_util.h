#pragma once

#include <filesystem>
#include <vector>

#include "config/tabular_data/input_table_type.h"
#include "parser/csv_parser/csv_parser.h"

namespace tests {

/// path to the directory with test data
static auto const test_data_dir = std::filesystem::current_path() / "input_data";

/// vector of csv configs
using CSVConfigs = std::vector<CSVConfig>;

/// a struct consisting of a csv config and the expected hash
struct CSVConfigHash {
    CSVConfig config;
    size_t hash;
};

/// a struct consisting of a csv config and the expected hash
struct CSVConfigsHash {
    CSVConfigs configs;
    size_t hash;
};

/// create input table from csv config
inline config::InputTable MakeInputTable(CSVConfig const& csv_config) {
    return std::make_shared<CSVParser>(csv_config);
}

}  // namespace tests
