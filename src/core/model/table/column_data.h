//
// Created by Ilya Vologin
// https://github.com/cupertank
//

#pragma once

#include <vector>

#include "abstract_column_data.h"
#include "column.h"
#include "position_list_index.h"
#include "position_list_index_with_singletons.h"

class ColumnData final : public model::AbstractColumnData {
    std::shared_ptr<model::PLIWithSingletons> position_list_index_;

public:
    ColumnData(Column const* column, std::unique_ptr<model::PLIWithSingletons> position_list_index)
        : AbstractColumnData(column), position_list_index_(std::move(position_list_index)) {
        position_list_index_->ForceCacheProbingTable();
    }

    ColumnData(Column const* column, std::unique_ptr<model::PositionListIndex> position_list_index)
        : AbstractColumnData(column) {
        position_list_index_ =
                std::make_shared<model::PLIWithSingletons>(*position_list_index.get());
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

    model::PLIWithSingletons const* GetPLWSIndex() const {
        return position_list_index_.get();
    }

    model::PLIWithSingletons* GetPLWSIndex() {
        return position_list_index_.get();
    }

    std::shared_ptr<model::PositionListIndex> GetPliOwnership() {
        return position_list_index_;
    }

    std::shared_ptr<model::PositionListIndex const> GetPliOwnership() const {
        return position_list_index_;
    }

    std::shared_ptr<model::PLIWithSingletons> GetPliwsOwnership() {
        return position_list_index_;
    }

    std::shared_ptr<model::PLIWithSingletons const> GetPliwsOwnership() const {
        return position_list_index_;
    }

    std::string ToString() const final {
        return "Data for " + column_->ToString();
    }

    static bool IsValueSingleton(int value) noexcept {
        return value == model::PLI::kSingletonValueId;
    }
};
