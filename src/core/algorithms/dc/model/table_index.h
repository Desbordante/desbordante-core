#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "core/algorithms/dc/model/component.h"
#include "core/model/table/dynamic_data/typed_dynamic_row_table_data.h"
#include "core/util/logger.h"
#include "core/util/lttree.h"

namespace algos::dc {

using EqualityIndex = std::unordered_map<Component, roaring::Roaring, Component::Hasher>;
using LessThanIndex = utils::LTTree<Component>;

class TableIndex {
    // for each column store its own index
    std::unordered_map<model::ColumnIndex, EqualityIndex> eq_index_;
    std::unordered_map<model::ColumnIndex, LessThanIndex> lt_index_;
    model::ITypedDynamicTableData const* table_;
    roaring::Roaring indexed_ids_;

    void DeleteFromIndices(size_t row_id, std::vector<std::byte const*> const& row) {
        auto types = table_->GetTypes();

        // Remove from LT index
        for (auto& [col, lt_tree] : lt_index_) {
            dc::Component val = {row[col], types[col]};
            lt_tree.Remove(val, row_id);
        }

        // Remove from equality index
        for (auto& [col, eq_map] : eq_index_) {
            dc::Component val = {row[col], types[col]};
            eq_map[val].remove(static_cast<uint32_t>(row_id));
        }
    }

    void InsertIntoIndices(size_t row_id) {
        auto types = table_->GetTypes();
        std::vector<std::byte const*> const& row = table_->GetRow(row_id);

        for (auto& [col, lt_tree] : lt_index_) {
            dc::Component val = {row[col], types[col], true};
            lt_tree.Insert(std::move(val), row_id);
        }
        for (auto& [col, eq_map] : eq_index_) {
            dc::Component val = {row[col], types[col], true};
            eq_map[std::move(val)].add(static_cast<uint32_t>(row_id));
        }
    }

public:
    TableIndex() = default;

    TableIndex(model::ITypedDynamicTableData const* table) : table_(table) {}

    void DeleteRow(size_t row_id, std::vector<std::byte const*> const& row) {
        indexed_ids_.remove(static_cast<uint32_t>(row_id));
        DeleteFromIndices(row_id, row);
    }

    void InsertRow(size_t row_id) {
        assert(table_->RowExists(row_id));
        indexed_ids_.add(static_cast<uint32_t>(row_id));
        InsertIntoIndices(row_id);
    }

    roaring::Roaring const& GetIndexedIds() const noexcept {
        return indexed_ids_;
    }

    bool IndexExists(model::ColumnIndex col, dc::OperatorType op) const {
        if (op == dc::OperatorType::kEqual || op == dc::OperatorType::kUnequal) {
            return eq_index_.contains(col);
        } else {
            return lt_index_.contains(col);
        }
    }

    void CreateIndex(model::ColumnIndex col, dc::OperatorType op) {
        if (IndexExists(col, op)) return;

        // Create appropriate empty index for each column
        if (op == dc::OperatorType::kEqual or op == dc::OperatorType::kUnequal) {
            eq_index_[col] = EqualityIndex();
        } else {
            lt_index_[col] = LessThanIndex();
        }
    }

    EqualityIndex const& GetEqualityIndex(model::ColumnIndex col) const {
        return eq_index_.at(col);
    }

    LessThanIndex const& GetLessThanIndex(model::ColumnIndex col) const {
        return lt_index_.at(col);
    }

    void Update() {
        auto const& inserted = table_->GetInserted();
        auto const& updated = table_->GetUpdated();
        auto const& deleted = table_->GetDeleted();

        // Process deleted rows - remove from cached data if available
        for (auto const& [row_id, row] : deleted) {
            assert(!table_->RowExists(row_id));
            DeleteFromIndices(row_id, row.GetData());
        }

        // Process inserted rows - add to indices
        for (size_t row_id : inserted) {
            assert(table_->RowExists(row_id));
            InsertIntoIndices(row_id);
        }

        // Process updated rows - remove old entries and add new ones
        for (auto const& [row_id, row] : updated) {
            assert(table_->RowExists(row_id));
            DeleteFromIndices(row_id, row.GetData());
            InsertIntoIndices(row_id);
        }
    }
};

}  // namespace algos::dc
