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
    std::vector<String> domain{};

    explicit StringValueRange(TypedColumnData const& column);
    StringValueRange(String value) : domain{std::move(value)} {};
    StringValueRange(std::vector<String> vec) : domain{std::move(vec)} {};

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

template <typename T>
class NumericValueRange : public ValueRange {
public:
    T lower_bound{};
    T upper_bound{};

    explicit NumericValueRange(TypedColumnData const& column) {
        bool initialized = false;
        for (size_t row_index = 0; row_index < column.GetNumRows(); ++row_index) {
            std::byte const* value = column.GetValue(row_index);
            T numeric_value = Type::GetValue<T>(value);
            if (!initialized) {
                lower_bound = numeric_value;
                upper_bound = numeric_value;
                initialized = true;
            } else {
                if (numeric_value < lower_bound) {
                    lower_bound = numeric_value;
                }
                if (numeric_value > upper_bound) {
                    upper_bound = numeric_value;
                }
            }
        }
    }

    explicit NumericValueRange(T lower_bound, T upper_bound)
        : lower_bound(lower_bound), upper_bound(upper_bound) {}

    [[nodiscard]] bool Includes(std::byte const* value) const override {
        T numeric_value = Type::GetValue<T>(value);
        return numeric_value >= lower_bound && numeric_value <= upper_bound;
    }

    TypeId GetTypeId() const override {
        if (std::is_same<T, Double>::value) {
            return TypeId::kDouble;
        } else if (std::is_same<T, Int>::value) {
            return TypeId::kInt;
        }
        throw std::logic_error("No TypeId corresponding to NumericValueRange's template type.");
    }

    [[nodiscard]] std::string ToString() const override {
        return "[" + std::to_string(lower_bound) + " - " + std::to_string(upper_bound) + "]";
    }

    ~NumericValueRange() = default;
};

std::shared_ptr<ValueRange> CreateValueRange(TypedColumnData const& column);

}  // namespace model
