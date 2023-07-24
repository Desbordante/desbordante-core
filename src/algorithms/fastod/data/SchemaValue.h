#pragma once

#include <string>
#include <cstddef>

#include "model/column_layout_typed_relation_data.h"

namespace algos::fastod {

class SchemaValue {
private:
    int index_in_column_;
    model::TypedColumnData const& column_data_;

public:
    SchemaValue(model::TypedColumnData const& column_data, size_t index_in_column) noexcept;

    std::string AsString() const;
    std::byte const* AsBytes() const noexcept;

    friend bool operator==(SchemaValue const& x, SchemaValue const& y);
    friend bool operator!=(SchemaValue const& x, SchemaValue const& y);
    friend bool operator<(SchemaValue const& x, SchemaValue const& y);
    friend bool operator<=(SchemaValue const& x, SchemaValue const& y);
    friend bool operator>(SchemaValue const& x, SchemaValue const& y);
    friend bool operator>=(SchemaValue const& x, SchemaValue const& y);
};

} // namespace algos::fastod
