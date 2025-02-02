#include "column_encoded_relation_data.h"

#include <memory>
#include <utility>

#include <easylogging++.h>

#include "table/table_index.h"

namespace model {
std::unique_ptr<ColumnEncodedRelationData> ColumnEncodedRelationData::CreateFrom(
        config::InputTable& data_stream, TableIndex table_id) {
    auto schema = std::make_unique<RelationalSchema>(data_stream->GetRelationName());
    std::unordered_map<std::string, int> value_dictionary;
    int next_value_id = 1;
    size_t const num_columns = data_stream->GetNumberOfColumns();
    std::vector<std::vector<int>> column_vectors = std::vector<std::vector<int>>(num_columns);
    std::vector<std::string> row;
    std::vector<std::set<std::string>> unique_values(data_stream->GetNumberOfColumns());

    while (data_stream->HasNextRow()) {
        row = data_stream->GetNextRow();

        if (row.size() != num_columns) {
            LOG(WARNING) << "Unexpected number of columns for a row, skipping (expected "
                         << num_columns << ", got " << row.size() << ")";
            continue;
        }

        for (size_t index = 0; index < row.size(); ++index) {
            std::string const& field = row[index];
            if (field.empty()) {
                column_vectors[index].push_back(kNullValueId);
                unique_values[index].insert(field);
            } else {
                int value_id;
                if (auto location = value_dictionary.find(field);
                    location == value_dictionary.end()) {
                    value_dictionary[field] = next_value_id;
                    unique_values[index].insert(field);
                    value_id = next_value_id;
                    next_value_id++;
                } else {
                    value_id = location->second;
                }
                column_vectors[index].push_back(value_id);
            }
        }
    }

    std::vector<EncodedColumnData> column_data;
    std::shared_ptr<std::unordered_map<int, std::string>> encoded_to_value(
            std::make_shared<std::unordered_map<int, std::string>>());
    for (auto& [value, encoded] : value_dictionary) {
        encoded_to_value->emplace(encoded, value);
    }

    for (size_t i = 0; i < num_columns; ++i) {
        auto column = Column(schema.get(), data_stream->GetColumnName(i), i);
        schema->AppendColumn(std::move(column));
        column_data.emplace_back(table_id, schema->GetColumn(i), std::move(column_vectors[i]),
                                 std::move(unique_values[i]), encoded_to_value);
    }

    schema->Init();

    return std::make_unique<ColumnEncodedRelationData>(std::move(schema), std::move(column_data),
                                                       std::move(encoded_to_value));
}
}  // namespace model
