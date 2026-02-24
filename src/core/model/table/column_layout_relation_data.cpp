//
// Created by Ilya Vologin
// https://github.com/cupertank
//
#include "core/model/table/column_layout_relation_data.h"

#include <cstddef>
#include <map>
#include <memory>
#include <utility>

#include "core/model/index.h"
#include "core/model/table/to_value_id_columns.h"
#include "core/util/logger.h"

std::vector<int> ColumnLayoutRelationData::GetTuple(int tuple_index) const {
    std::size_t num_columns = column_data_.size();
    std::vector<int> tuple = std::vector<int>(num_columns);
    for (model::Index column_index = 0; column_index < num_columns; column_index++) {
        tuple[column_index] = column_data_[column_index].GetProbingTableValue(tuple_index);
    }
    return tuple;
}

std::shared_ptr<model::PLI const> ColumnLayoutRelationData::CalculatePLI(
        std::vector<model::Index> const& indices) const {
    if (indices.size() <= 0) throw std::invalid_argument("received unpositive number of indices");

    std::shared_ptr<model::PLI const> pli = GetColumnData(indices[0]).GetPliOwnership();

    for (size_t i = 1; i < indices.size(); ++i) {
        pli = pli->Intersect(GetColumnData(indices[i]).GetPositionListIndex());
    }
    return pli;
}

std::shared_ptr<model::PLIWS const> ColumnLayoutRelationData::CalculatePLIWS(
        std::vector<unsigned int> const& indices) const {
    if (indices.size() <= 0) throw std::invalid_argument("received unpositive number of indices");

    std::shared_ptr<model::PLIWS const> pliws = GetColumnData(indices[0]).GetPliwsOwnership();

    for (size_t i = 1; i < indices.size(); ++i) {
        pliws = pliws->Intersect(GetColumnData(indices[i]).GetPLWSIndex());
    }
    return pliws;
}

std::unique_ptr<ColumnLayoutRelationData> ColumnLayoutRelationData::CreateFrom(
        model::IDatasetStream& data_stream) {
    size_t const num_columns = data_stream.GetNumberOfColumns();
    std::vector<model::ValueIdColumn> value_id_columns = ToValueIdColumns(data_stream);

    auto schema = RelationalSchema::CreateFrom(data_stream);
    std::vector<ColumnData> column_data;
    for (size_t i = 0; i < num_columns; ++i) {
        auto pli = model::PLIWithSingletons::CreateFor(value_id_columns[i]);
        column_data.emplace_back(schema->GetColumn(i), std::move(pli));
    }

    return std::make_unique<ColumnLayoutRelationData>(std::move(schema), std::move(column_data));
}
