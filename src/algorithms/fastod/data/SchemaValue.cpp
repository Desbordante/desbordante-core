#include <string>
#include <cstddef>

#include "model/column_layout_typed_relation_data.h"

#include "SchemaValue.h"

using namespace algos::fastod;

bool SchemaValue::is_empty_equal_empty_ = true;
util::config::EqNullsType SchemaValue::is_null_equal_null_ = true;

SchemaValue::SchemaValue(model::TypeId type_id, std::byte const* value) noexcept
    : type_id_(type_id), value_(value) { }

model::TypeId SchemaValue::GetTypeId() const noexcept {
    return type_id_;
}

std::byte const* SchemaValue::GetValue() const noexcept {
    return value_;
}

std::string SchemaValue::ToString() const {
    switch (type_id_) {
        case model::TypeId::kInt: return std::to_string(AsInt());
        case model::TypeId::kDouble: return std::to_string(AsDouble());
        case model::TypeId::kString: return AsString();
        case model::TypeId::kBigInt: return AsBigInt();
        case model::TypeId::kEmpty: return model::EmptyType().ValueToString(value_);
        case model::TypeId::kNull: return model::NullType(true).ValueToString(value_);
    }

    return std::string();
}

int SchemaValue::AsInt() const {
    return *reinterpret_cast<int const*>(value_);
}

double SchemaValue::AsDouble() const {
    return *reinterpret_cast<double const*>(value_);
}

BigInt SchemaValue::AsBigInt() const {
    return BigInt(AsString());
}

std::string SchemaValue::AsString() const {
    return model::StringType().ValueToString(value_);
}

bool SchemaValue::IsInt() const noexcept {
    return GetTypeId() == +model::TypeId::kInt;
}

bool SchemaValue::IsDouble() const noexcept {
    return GetTypeId() == +model::TypeId::kDouble;
}

bool SchemaValue::IsBigInt() const noexcept {
    return GetTypeId() == +model::TypeId::kBigInt;
}

bool SchemaValue::IsString() const noexcept {
    return GetTypeId() == +model::TypeId::kString;
}

bool SchemaValue::IsEmpty() const noexcept {
    return GetTypeId() == +model::TypeId::kEmpty;
}

bool SchemaValue::IsNull() const noexcept {
    return GetTypeId() == +model::TypeId::kNull;
}

bool SchemaValue::IsNumeric() const noexcept {
    return IsInt() || IsDouble();
}

SchemaValue SchemaValue::FromTypedColumnData(model::TypedColumnData const& column, size_t index) noexcept {
    return SchemaValue(column.GetValueTypeId(index), column.GetValue(index));
}

namespace algos::fastod {

bool operator==(SchemaValue const& x, SchemaValue const& y) {
    if (x.type_id_ != y.type_id_)
        return x.ToString() == y.ToString();
    
    switch (x.type_id_) {
        case model::TypeId::kInt: return x.AsInt() == y.AsInt();
        case model::TypeId::kDouble: return x.AsDouble() == y.AsDouble();
        case model::TypeId::kBigInt: return x.AsBigInt() == y.AsBigInt();
        case model::TypeId::kString: return x.AsString() == y.AsString();     
        case model::TypeId::kEmpty: return SchemaValue::is_empty_equal_empty_;

        case model::TypeId::kNull: return model::NullType(SchemaValue::is_null_equal_null_)
            .Compare(x.value_, y.value_) == model::CompareResult::kEqual;
    }

    return false;
}

bool operator!=(SchemaValue const& x, SchemaValue const& y) {
    return !(x == y);
}

bool operator<(SchemaValue const& x, SchemaValue const& y) {
    if (x.type_id_ != y.type_id_)
        return x.ToString() < y.ToString();
    
    switch (x.type_id_) {
        case model::TypeId::kInt: return x.AsInt() < y.AsInt();
        case model::TypeId::kDouble: return x.AsDouble() < y.AsDouble();
        case model::TypeId::kBigInt: return x.AsBigInt() < y.AsBigInt();
        case model::TypeId::kString: return x.AsString() < y.AsString();     
        case model::TypeId::kEmpty: return false;

        case model::TypeId::kNull: return model::NullType(SchemaValue::is_null_equal_null_)
            .Compare(x.value_, y.value_) == model::CompareResult::kLess;
    }

    return false;
}

bool operator<=(SchemaValue const& x, SchemaValue const& y) {
    return (x < y) || (x == y);
}

bool operator>(SchemaValue const& x, SchemaValue const& y) {
    return !(x <= y);
}

bool operator>=(SchemaValue const& x, SchemaValue const& y) {
    return !(x < y);
}

} // namespace algos::fastod
