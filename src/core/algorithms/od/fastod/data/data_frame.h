#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include "config/equal_nulls/type.h"
#include "model/table/column_layout_typed_relation_data.h"

#include "schema_value.h"

namespace algos::fastod {

class DataFrame {
private:
    std::vector<model::TypedColumnData> columns_data_;

public:
    DataFrame() = default;
    DataFrame(const DataFrame&) = delete;
    explicit DataFrame(std::vector<model::TypedColumnData> columns_data) noexcept;

    SchemaValue GetValue(int tuple_index, int attribute_index) const noexcept;
    
    std::size_t GetColumnCount() const noexcept;
    std::size_t GetTupleCount() const noexcept;

    static DataFrame FromCsv(std::filesystem::path const& path);

    static DataFrame FromCsv(std::filesystem::path const& path,
                             char separator,
                             bool has_header,
                             config::EqNullsType is_null_equal_null);

};

} // namespace algos::fatod
