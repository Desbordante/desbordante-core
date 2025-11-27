#include "core/model/table/column_layout_typed_relation_data.h"

#include "core/util/logger.h"

namespace model {

std::unique_ptr<ColumnLayoutTypedRelationData> ColumnLayoutTypedRelationData::CreateFrom(
        IDatasetStream& data_stream, bool is_null_eq_null, bool treat_mixed_as_string) {
    auto schema = std::make_unique<RelationalSchema>(data_stream.GetRelationName());
    size_t const num_columns = data_stream.GetNumberOfColumns();

    std::vector<std::vector<std::string>> columns(num_columns);
    std::vector<std::string> row;

    /* Parsing is very similar to ColumnLayoutRelationData::CreateFrom().
     * Maybe we need column-based parsing in addition to row-based in CSVParser
     * (now IDatasetStream) */
    while (data_stream.HasNextRow()) {
        row = data_stream.GetNextRow();

        if (row.size() != num_columns) {
            LOG_WARN(
                    "Unexpected number of columns for a row, "
                    "skipping (expected {}, got {})",
                    num_columns, row.size());
            continue;
        }

        for (size_t index = 0; index < row.size(); ++index) {
            std::string& field = row[index];
            columns[index].push_back(std::move(field));
        }
    }

    std::vector<TypedColumnData> column_data;
    for (size_t i = 0; i < num_columns; ++i) {
        Column column(schema.get(), data_stream.GetColumnName(i), i);
        schema->AppendColumn(std::move(column));
        TypedColumnData typed_column_data = model::TypedColumnDataFactory::CreateFrom(
                schema->GetColumn(i), std::move(columns[i]), is_null_eq_null,
                treat_mixed_as_string);
        column_data.emplace_back(std::move(typed_column_data));
    }

    return std::make_unique<ColumnLayoutTypedRelationData>(std::move(schema),
                                                           std::move(column_data));
}

}  // namespace model
