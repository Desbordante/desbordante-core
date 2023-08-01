#pragma once

#include "numeric_type.h"
#include "cast/cast_from_int.h"
#include "double_type.h"
namespace model {
class IntType final : public NumericType<Int> {
public:
    IntType() noexcept : NumericType<Int>(TypeId::kInt) {}

    [[nodiscard]] std::unique_ptr<Type> CloneType() const override {
        return std::make_unique<IntType>();
    }
    ICastToCppType& CastToBuiltin() override {
        return this->caster_to_builtin_;
    }
    ICastToNumericType& CastToNumeric() override {
        return this->caster_to_numeric_;
    }
    
protected:
    model::CastFromIntType caster_to_builtin_;
    CastFromIntTypeToNumeric caster_to_numeric_;
};

}  // namespace model
