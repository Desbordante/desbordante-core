#include "column_encoded_relation_data.h"

#include <memory>
#include <utility>

#include <easylogging++.h>

#include "table/table_index.h"

namespace model {
std::unique_ptr<ColumnEncodedRelationData> ColumnEncodedRelationData::CreateFrom(
        config::InputTable& data_stream, TableIndex table_id,
        ValueDictionaryType value_dictionary) {
    auto schema = std::make_unique<RelationalSchema>(data_stream->GetRelationName());
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
            unique_values[index].insert(field);
            if (field.empty()) {
                column_vectors[index].push_back(kNullValueId);
            } else {
                column_vectors[index].push_back(value_dictionary->ToInt(field));
            }
        }
    }

    std::vector<EncodedColumnData> column_data;
    for (size_t i = 0; i < num_columns; ++i) {
        auto column = Column(schema.get(), data_stream->GetColumnName(i), i);
        schema->AppendColumn(std::move(column));
        column_data.emplace_back(table_id, schema->GetColumn(i), std::move(column_vectors[i]),
                                 std::move(unique_values[i]), value_dictionary);
    }
    schema->Init();
    return std::make_unique<ColumnEncodedRelationData>(std::move(schema), std::move(column_data));
}
}  // namespace model
