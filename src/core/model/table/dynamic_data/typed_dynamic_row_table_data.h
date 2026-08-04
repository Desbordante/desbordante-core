#pragma once

#include <algorithm>
#include <bitset>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/regex.hpp>

#include "core/config/exceptions.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/dynamic_data/dynamic_table_data.h"
#include "core/model/table/dynamic_data/row.h"
#include "core/model/table/typed_column_data.h"
#include "core/model/types/create_type.h"
#include "core/model/types/type_deduction.h"
#include "core/util/logger.h"

namespace model {

// Should probably be named *RelationData, but in order to avoid misconception stays as is
// TODO (Anosov Pavel): Create single interface for all types of
// table data (static, dynamic) and fix naming
class TypedDynamicRowTableData : public ITypedDynamicTableData
// , std::enable_shared_from_this<TypedDynamicRowTableData>
{
private:
    std::unordered_map<size_t, Row> rows_;
    std::vector<std::shared_ptr<Type>> types_;
    std::set<size_t> all_ids_;
    size_t num_cols_ = 0;
    size_t max_row_id_ = 0;
    size_t row_size_ = 0;

    // TODO (Anosov Pavel): Avoid transforming table to column data
    void InferTypes(IDatasetStream& input_table) {
        bool treat_mixed_as_string = true;
        bool is_null_equal_null = true;

        std::vector<std::vector<std::string>> columns(num_cols_);
        while (input_table.HasNextRow()) {
            std::vector<std::string> row = input_table.GetNextRow();
            if (row.size() != num_cols_) continue;
            for (size_t col = 0; col < num_cols_; ++col) {
                columns[col].push_back(std::move(row[col]));
            }
        }

        types_.reserve(num_cols_);
        for (size_t col = 0; col < num_cols_; ++col) {
            TypeId deduced_type_id = DeduceColumnType(columns[col], treat_mixed_as_string);
            std::shared_ptr<Type> type = CreateType(deduced_type_id, is_null_equal_null);
            types_.push_back(std::move(type));
        }

        row_size_ = [this]() -> size_t {
            size_t row_size = 0;
            for (auto const& type : types_) row_size += type->GetSize();
            return row_size;
        }();
    }

    void AppendRow(std::vector<std::string> const& values, bool track_insert) {
        assert(types_.size() > 0);  // Call only after types are deduced
        assert(values.size() == num_cols_);
        assert(!rows_.contains(max_row_id_));

        size_t const row_id = max_row_id_++;
        Row row = {values, types_, row_size_};
        rows_.emplace(row_id, std::move(row));
        if (track_insert) inserted_.insert(row_id);
        all_ids_.insert(row_id);
        // LOG_DEBUG("Append row: {}", row_id);
    }

public:
    // TODO (Anosov Pavel): Implement mixed type support
    explicit TypedDynamicRowTableData(IDatasetStream& input_table, size_t init_offset = 0)
        : num_cols_(input_table.GetNumberOfColumns()), max_row_id_(init_offset) {
        InferTypes(input_table);
        input_table.Reset();

        while (input_table.HasNextRow()) {
            IDatasetStream::Row row = input_table.GetNextRow();
            AppendRow(row, false);
        }
    }

    size_t GetNumRows() const override {
        return rows_.size();
    }

    size_t GetNumCols() const override {
        return num_cols_;
    }

    bool RowExists(size_t row) const override {
        return rows_.contains(row);
    }

    // Return values may invalidate on Update()
    std::byte const* GetValue(size_t row, size_t col) const override {
        assert(col < GetNumCols() and rows_.contains(row));
        return rows_.at(row).GetValue(col);
    }

    void DeleteRow(size_t row) override {
        assert(rows_.contains(row));
        deleted_.emplace(row, std::move(rows_.at(row)));
        rows_.erase(row);
        all_ids_.erase(row);
    }

    void AppendRow(std::vector<std::string> const& values) override {
        AppendRow(values, true);
    }

    void UpdateRow(size_t row, std::vector<std::string> const& values) override {
        assert(rows_.contains(row));
        assert(!types_.empty());
        updated_.emplace(row, rows_.at(row));
        rows_.at(row).Update(values, types_);
    }

    // Return values may invalidate on Update()
    std::vector<std::byte const*> GetRow(size_t row) const override {
        assert(rows_.contains(row));
        return rows_.at(row).GetData();
    }

    // Return values may invalidate on Update()
    std::vector<std::byte const*> GetCol(size_t col) const override {
        assert(col < GetNumCols());
        std::vector<std::byte const*> res;
        for (size_t row : all_ids_) {
            res.push_back(rows_.at(row).GetValue(col));
        }

        return res;
    }

    std::vector<model::Type const*> GetTypes() const override {
        std::vector<model::Type const*> res;
        for (auto const& type : types_) res.push_back(type.get());
        return res;
    }

    std::set<size_t> GetAllIds() const override {
        return all_ids_;
    }
};

}  // namespace model
