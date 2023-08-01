#pragma once

#include "numeric_type.h"

//#include "int_type.h"

#include "cast/cast_from_double.h"
namespace model {
class DoubleType final : public NumericType<Double> {
public:
    DoubleType() noexcept : NumericType<Double>(TypeId::kDouble) {}

    CompareResult Compare(std::byte const* l, std::byte const* r) const final {
        Double l_val = GetValue(l);
        Double r_val = GetValue(r);

        if (std::abs(l_val - r_val) < std::numeric_limits<Double>::epsilon()) {
            return CompareResult::kEqual;
        }

        if (l_val < r_val) {
            return CompareResult::kLess;
        }

        return CompareResult::kGreater;
    }

    [[nodiscard]] std::unique_ptr<Type> CloneType() const override {
        return std::make_unique<DoubleType>();
    }

    static std::byte* MakeFrom(const std::byte* data, const Type& type) {
        switch (type.GetTypeId()) {
        case TypeId::kDouble:
            return DoubleType().Clone(data);
        case TypeId::kInt:
            return DoubleType().MakeValue(Type::GetValue<Int>(data));
        default:
            return nullptr;
        }
    }
    ICastToCppType& CastToBuiltin() override {
        return this->caster_to_builtin_;
    }
    ICastToNumericType& CastToNumeric() override {
        return this->caster_to_numeric_;
    }

protected:
    model::CastFromDoubleType caster_to_builtin_;
    CastFromDoubleTypeToNumeric caster_to_numeric_;
};
}  // namespace model
