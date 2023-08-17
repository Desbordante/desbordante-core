#include <vector>
#include <string>
#include <utility>
#include <filesystem>

#include "parser/csv_parser/csv_parser.h"
#include "config/equal_nulls/type.h"
#include "model/table/column_layout_typed_relation_data.h"

#include "data_frame.h"
#include "schema_value.h"

using namespace algos::fastod;

DataFrame::DataFrame(std::vector<model::TypedColumnData> columns_data) noexcept
    : columns_data_(std::move(columns_data)) { }

SchemaValue DataFrame::GetValue(int tuple_index, int attribute_index) const noexcept {
    return SchemaValue::FromTypedColumnData(columns_data_.at(attribute_index), tuple_index);
}

std::size_t DataFrame::GetColumnCount() const noexcept {
    return columns_data_.size();
}

std::size_t DataFrame::GetTupleCount() const noexcept {
    return columns_data_.size() > 0
        ? columns_data_.at(0).GetNumRows()
        : 0;
}

DataFrame DataFrame::FromCsv(std::filesystem::path const& path) {
    return FromCsv(path, ',', true, true);
}

DataFrame DataFrame::FromCsv(std::filesystem::path const& path,
                             char separator,
                             bool has_header,
                             config::EqNullsType is_null_equal_null) {
    CSVParser parser = CSVParser(path, separator, has_header);
    std::vector<model::TypedColumnData> columns_data = model::CreateTypedColumnData(parser, is_null_equal_null);

    return DataFrame(std::move(columns_data));
}
