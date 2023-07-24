#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include "util/config/equal_nulls/type.h"
#include "model/column_layout_typed_relation_data.h"

#include "SchemaValue.h"

namespace algos::fastod {

class DataFrame {
private:
    std::vector<model::TypedColumnData> columns_data_;

public:
    explicit DataFrame(std::vector<model::TypedColumnData> columns_data) noexcept;

    SchemaValue const& GetValue(int tuple_index, int attribute_index) const noexcept;
    
    size_t GetColumnCount() const noexcept;
    size_t GetTupleCount() const noexcept;

    static DataFrame FromCsv(std::filesystem::path const& path);

    static DataFrame FromCsv(std::filesystem::path const& path,
                             char separator,
                             bool has_header,
                             util::config::EqNullsType is_null_equal_null);
};

} // namespace algos::fatod
