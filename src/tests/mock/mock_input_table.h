
#pragma once

#include <string>
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

using namespace algos;
using namespace config::names;

// ---------------------------------------------------------------------------
// In-memory IDatasetStream — use instead of CSV files for CRUD tables
// ---------------------------------------------------------------------------

namespace tests {

class MockTable : public model::IDatasetStream {
    std::string name_;
    std::vector<std::string> columns_;
    std::vector<Row> rows_;
    size_t pos_ = 0;

public:
    MockTable(std::string name, std::vector<std::string> columns, std::vector<Row> rows)
        : name_(std::move(name)), columns_(std::move(columns)), rows_(std::move(rows)) {}

    Row GetNextRow() override {
        return rows_[pos_++];
    }

    bool HasNextRow() const override {
        return pos_ < rows_.size();
    }

    size_t GetNumberOfColumns() const override {
        return columns_.size();
    }

    std::string GetColumnName(size_t i) const override {
        return columns_[i];
    }

    std::string GetRelationName() const override {
        return name_;
    }

    void Reset() override {
        pos_ = 0;
    }
};

}  // namespace tests
