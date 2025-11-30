#pragma once

#include <enum.h>

#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/type.h"

namespace model {

class ValueRange {
protected:
    ValueRange() = default;

public:
    virtual TypeId GetTypeId() const = 0;
    virtual bool Includes(std::byte const* value) const = 0;
    virtual std::string ToString() const = 0;
    virtual ~ValueRange() = 0;
};

struct StringValueRange : public ValueRange {
    std::vector<String> domain{};

    explicit StringValueRange(TypedColumnData const& column);
    StringValueRange(String value) : domain{std::move(value)} {};
    StringValueRange(std::vector<String> vec) : domain{std::move(vec)} {};

    TypeId GetTypeId() const override {
        return TypeId::kString;
    }

    bool Includes(std::byte const* value) const override;

    std::string ToString() const override;
};

template <typename T>
struct NumericValueRange : public ValueRange {
    T lower_bound{};
    T upper_bound{};

    explicit NumericValueRange(TypedColumnData const& column) {
        auto [min_ptr, max_ptr] = std::ranges::minmax(
                column.GetData(), {},
                [](std::byte const* value) { return Type::GetValue<T>(value); });
        lower_bound = Type::GetValue<T>(min_ptr);
        upper_bound = Type::GetValue<T>(max_ptr);
    }

    explicit NumericValueRange(T lower_bound, T upper_bound)
        : lower_bound(lower_bound), upper_bound(upper_bound) {}

    [[nodiscard]] bool Includes(std::byte const* value) const override {
        T numeric_value = Type::GetValue<T>(value);
        return numeric_value >= lower_bound && numeric_value <= upper_bound;
    }

    TypeId GetTypeId() const override {
        if constexpr (std::is_same_v<T, Double>) {
            return TypeId::kDouble;
        } else if constexpr (std::is_same_v<T, Int>) {
            return TypeId::kInt;
        }
        throw std::logic_error("No TypeId corresponding to NumericValueRange's template type.");
    }

    [[nodiscard]] std::string ToString() const override {
        return "[" + std::to_string(lower_bound) + " - " + std::to_string(upper_bound) + "]";
    }
};

std::shared_ptr<ValueRange> CreateValueRange(TypedColumnData const& column);

}  // namespace model
