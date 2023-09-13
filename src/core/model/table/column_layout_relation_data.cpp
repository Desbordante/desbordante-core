//
// Created by Ilya Vologin
// https://github.com/cupertank
//
#include "column_layout_relation_data.h"

#include <map>
#include <memory>
#include <utility>

#include <easylogging++.h>

std::vector<int> ColumnLayoutRelationData::GetTuple(int tuple_index) const {
    int num_columns = schema_->GetNumColumns();
    std::vector<int> tuple = std::vector<int>(num_columns);
    for (int column_index = 0; column_index < num_columns; column_index++) {
        tuple[column_index] = column_data_[column_index].GetProbingTableValue(tuple_index);
    }
    return tuple;
}

std::unique_ptr<ColumnLayoutRelationData> ColumnLayoutRelationData::CreateFrom(
        model::IDatasetStream& data_stream, bool is_null_eq_null) {
    auto schema = std::make_unique<RelationalSchema>(data_stream.GetRelationName());
    std::unordered_map<std::string, int> value_dictionary;
    int next_value_id = 1;
    const int null_value_id = kNullValueId;
    const size_t num_columns = data_stream.GetNumberOfColumns();
    std::vector<std::vector<int>> column_vectors = std::vector<std::vector<int>>(num_columns);
    std::vector<std::string> row;

    while (data_stream.HasNextRow()) {
        row = data_stream.GetNextRow();

        if (row.empty() && num_columns == 1) {
            row.emplace_back("");
        } else if (row.size() != num_columns) {
            LOG(WARNING) << "Skipping incomplete rows";
            continue;
        }

        size_t index = 0;
        for (std::string& field : row) {
            if (field.empty()) {
                column_vectors[index].push_back(null_value_id);
            } else {
                auto location = value_dictionary.find(field);
                int value_id;
                if (location == value_dictionary.end()) {
                    value_dictionary[field] = next_value_id;
                    value_id = next_value_id;
                    next_value_id++;
                } else {
                    value_id = location->second;
                }
                column_vectors[index].push_back(value_id);
            }
            index++;
            if (index >= num_columns) break;
        }
    }

    std::vector<ColumnData> column_data;
    for (size_t i = 0; i < num_columns; ++i) {
        auto column = Column(schema.get(), data_stream.GetColumnName(i), i);
        schema->AppendColumn(std::move(column));
        auto pli = model::PositionListIndex::CreateFor(column_vectors[i], is_null_eq_null);
        column_data.emplace_back(schema->GetColumn(i), std::move(pli));
    }

    schema->Init();

    return std::make_unique<ColumnLayoutRelationData>(std::move(schema), std::move(column_data));
}
