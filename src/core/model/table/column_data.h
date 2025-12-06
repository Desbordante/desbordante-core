//
// Created by Ilya Vologin
// https://github.com/cupertank
//

#pragma once

#include <vector>

#include "core/model/table/abstract_column_data.h"
#include "core/model/table/column.h"
#include "core/model/table/position_list_index.h"

class ColumnData final : public model::AbstractColumnData {
    std::shared_ptr<model::PositionListIndex> position_list_index_;

public:
    ColumnData(Column const* column, std::unique_ptr<model::PositionListIndex> position_list_index)
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
        return GetProbingTableValue(tuple_index) == model::PLI::kSingletonValueId;
    }

    model::PositionListIndex const* GetPositionListIndex() const {
        return position_list_index_.get();
    }

    model::PositionListIndex* GetPositionListIndex() {
        return position_list_index_.get();
    }

    std::shared_ptr<model::PositionListIndex> GetPliOwnership() {
        return position_list_index_;
    }

    std::shared_ptr<model::PositionListIndex const> GetPliOwnership() const {
        return position_list_index_;
    }

    std::string ToString() const final {
        return "Data for " + column_->ToString();
    }

    static bool IsValueSingleton(int value) noexcept {
        return value == model::PLI::kSingletonValueId;
    }
};
