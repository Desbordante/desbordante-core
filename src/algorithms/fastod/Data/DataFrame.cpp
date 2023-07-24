#include <vector>
#include <string>
#include <filesystem>

#include "parser/csv_parser.h"
#include "util/config/equal_nulls/type.h"
#include "model/column_layout_typed_relation_data.h"

#include "DataFrame.h"
#include "SchemaValue.h"

using namespace algos::fastod;

DataFrame::DataFrame(std::vector<model::TypedColumnData> columns_data) noexcept
    : columns_data_(std::move(columns_data)) { }

SchemaValue const& DataFrame::GetValue(int tuple_index, int attribute_index) const noexcept {
    return SchemaValue(columns_data_.at(attribute_index), tuple_index);
}

DataFrame DataFrame::FromCsv(std::filesystem::path const& path) {
    return FromCsv(path, ',', true, true);
}

DataFrame DataFrame::FromCsv(std::filesystem::path const& path,
                             char separator,
                             bool has_header,
                             util::config::EqNullsType is_null_equal_null) {
    CSVParser parser = CSVParser(path, separator, has_header);
    std::vector<model::TypedColumnData> columns_data = model::CreateTypedColumnData(parser, is_null_equal_null);

    return DataFrame(std::move(columns_data));
}
