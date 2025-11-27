#pragma once

#include <cassert>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/config/exceptions.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/util/logger.h"

namespace model {

struct DynamicTableData {
private:
    std::vector<std::vector<std::string>> columns_;
    std::unordered_set<size_t> deleted_rows_{};

public:
    DynamicTableData(IDatasetStream& input_table) {
        columns_.resize(input_table.GetNumberOfColumns());
        while (input_table.HasNextRow()) {
            std::vector<std::string> row = input_table.GetNextRow();
            if (row.size() != columns_.size()) {
                LOG_DEBUG("Got input table row with {} size, skipping...", row.size());
                continue;
            }
            for (size_t i = 0; i < row.size(); ++i) {
                columns_[i].push_back(std::move(row[i]));
            }
        };
    }

    size_t GetNumRowsActual() const {
        return columns_[0].size() - deleted_rows_.size();
    }

    size_t GetNumRowsTotal() const {
        return columns_[0].size();
    }

    std::string const& GetValue(size_t row_index, size_t col_index) const {
        assert(col_index < columns_.size() && row_index < columns_[col_index].size());
        return columns_[col_index][row_index];
    }

    bool IsRowIndexValid(size_t row_index) {
        return !(deleted_rows_.contains(row_index) || row_index >= GetNumRowsTotal());
    }

    void Update(config::InputTable insert_data, config::InputTable update_data,
                std::unordered_set<size_t> const& delete_data) {
        for (size_t row_id : delete_data) {
            for (size_t i = 0; i < columns_.size(); ++i) {
                columns_[i][row_id] = {};
            }
            deleted_rows_.emplace(row_id);
        }
        if (insert_data != nullptr) {
            while (insert_data->HasNextRow()) {
                std::vector<std::string> row = insert_data->GetNextRow();
                if (row.size() != columns_.size()) {
                    LOG_DEBUG("Got insert statement row with {}  size, skipping...", row.size());
                    continue;
                }
                for (size_t i = 0; i < row.size(); ++i) {
                    columns_[i].push_back(std::move(row[i]));
                }
            }
        }
        if (update_data != nullptr) {
            while (update_data->HasNextRow()) {
                std::vector<std::string> row = update_data->GetNextRow();
                if (row.size() != columns_.size() + 1) {
                    LOG_DEBUG("Got update statement row with {}  size, skipping...", row.size());
                    continue;
                }
                size_t row_id = std::stoull(row.front());
                if (deleted_rows_.contains(row_id)) {
                    throw config::ConfigurationError(
                            "Attempt to update a deleted row during processing of update "
                            "operations");
                }
                for (size_t i = 1; i < row.size(); ++i) {
                    columns_[i - 1][row_id] = std::move(row[i]);
                }
            }
        }
    }
};

}  // namespace model
