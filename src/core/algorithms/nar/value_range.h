#pragma once

#include <enum.h>

#include "model/table/column_layout_typed_relation_data.h"
#include "model/types/type.h"

namespace model {

class ValueRange {
protected:
    ValueRange() {};
    virtual ~ValueRange() = default;
public:
    virtual TypeId GetTypeId() const = 0;
    virtual bool Includes(std::byte const* value) const = 0;
    virtual std::string ToString() const = 0;
};

class StringValueRange : public ValueRange {
public:
    std::vector<String> domain;
    explicit StringValueRange(TypedColumnData const& column);

    StringValueRange(String value)
    : domain{std::move(value)} {};

    TypeId GetTypeId() const override {
        return TypeId::kString;
    }

    bool Includes(std::byte const* value) const override {
        String const& svalue = Type::GetValue<String>(value);
        return std::find(domain.begin(), domain.end(), svalue) != domain.end();
    }

    std::string ToString() const override;
    ~StringValueRange() = default;
};

class DoubleValueRange : public ValueRange {
public:
    Double lower_bound;
    Double upper_bound;
    explicit DoubleValueRange(TypedColumnData const& column);
    DoubleValueRange(Double lower_bound, Double upper_bound)
        : lower_bound(lower_bound), upper_bound(upper_bound) {};

    TypeId GetTypeId() const override {
        return TypeId::kDouble;
    }

    bool Includes(std::byte const* value) const override {
        Double dvalue = Type::GetValue<Double>(value);
        return dvalue >= lower_bound && dvalue <= upper_bound;
    }

    std::string ToString() const override;
    ~DoubleValueRange() = default;
};

class IntValueRange : public ValueRange {
public:
    Int lower_bound;
    Int upper_bound;

    TypeId GetTypeId() const override {
        return TypeId::kInt;
    }

    bool Includes(std::byte const* value) const override {
        Int ivalue = Type::GetValue<Int>(value);
        return ivalue >= lower_bound && ivalue <= upper_bound;
    }

    std::string ToString() const override;
    explicit IntValueRange(TypedColumnData const& column);
    IntValueRange(Int lower_bound, Int upper_bound)
        : lower_bound(lower_bound), upper_bound(upper_bound) {};
    ~IntValueRange() = default;
};

std::shared_ptr<ValueRange> CreateValueRange(TypedColumnData const& column);

}  // namespace model
