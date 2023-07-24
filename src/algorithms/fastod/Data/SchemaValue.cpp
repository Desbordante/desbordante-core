#include <string>
#include <utility>
#include <cstddef>

#include "model/column_layout_typed_relation_data.h"

#include "SchemaValue.h"

using namespace algos::fastod;

SchemaValue::SchemaValue(model::TypedColumnData const& column_data,
                         size_t index_in_column) noexcept
    : index_in_column_(index_in_column),
      column_data_(std::move(column_data)) { }

std::string SchemaValue::AsString() const {
    return column_data_.GetDataAsString(index_in_column_);
}

std::byte const* SchemaValue::AsBytes() const noexcept {
    return column_data_.GetValue(index_in_column_);
}

bool algos::fastod::operator==(SchemaValue const& x, SchemaValue const& y) {
    return false; // TODO
}

bool algos::fastod::operator!=(SchemaValue const& x, SchemaValue const& y) {
    return !(x == y);
}

bool algos::fastod::operator<(SchemaValue const& x, SchemaValue const& y) {
    return false; // TODO
}

bool algos::fastod::operator<=(SchemaValue const& x, SchemaValue const& y) {
    return (x < y) || (x == y);
}

bool algos::fastod::operator>(SchemaValue const& x, SchemaValue const& y) {
    return !(x <= y);
}

bool algos::fastod::operator>=(SchemaValue const& x, SchemaValue const& y) {
    return !(x < y);
}
