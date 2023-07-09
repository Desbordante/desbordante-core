//
// Created by Ilya Vologin
// https://github.com/cupertank
//

#pragma once

#include <vector>

#include "abstract_column_data.h"
#include "column.h"
#include "structures/position_list_index.h"

class ColumnData final : public model::AbstractColumnData {
    std::shared_ptr<structures::PositionListIndex> position_list_index_;

public:
    ColumnData(Column const* column,
               std::unique_ptr<structures::PositionListIndex> position_list_index)
        : AbstractColumnData(column), position_list_index_(std::move(position_list_index)) {
        position_list_index_->ForceCacheProbingTable();
    }
    // Инвариант: конструктором гарантируется, что в ColumnData.PLI есть закешированная ProbingTable
    std::vector<int> const& GetProbingTable() const {
        return *position_list_index_->GetCachedProbingTable();
    }
    int GetProbingTableValue(int tuple_index) const {
        return (*position_list_index_->GetCachedProbingTable())[tuple_index];
    }
    bool IsSingleton(int tuple_index) const noexcept {
        return GetProbingTableValue(tuple_index) == structures::PLI::singleton_value_id_;
    }
    structures::PositionListIndex const* GetPositionListIndex() const {
        return position_list_index_.get();
    }
    structures::PositionListIndex* GetPositionListIndex() {
        return position_list_index_.get();
    }

    std::shared_ptr<structures::PositionListIndex> GetPliOwnership() {
        return position_list_index_;
    }
    std::shared_ptr<structures::PositionListIndex const> GetPliOwnership() const {
        return position_list_index_;
    }

    std::string ToString() const final {
        return "Data for " + column_->ToString();
    }

    static bool IsValueSingleton(int value) noexcept {
        return value == structures::PLI::singleton_value_id_;
    }
};
