#pragma once

#include "numeric_type.h"

namespace model {

class IntType final : public NumericType<Int> {
public:
    IntType() noexcept : NumericType<Int>(TypeId::kInt) {}

    [[nodiscard]] std::unique_ptr<Type> CloneType() const override {
        return std::make_unique<IntType>();
    }
    virtual ICastToCppType & CastToBuiltin(){
        return this->caster_to_builtin_;
    }
    protected:
    model::CastFromIntType caster_to_builtin_;
};

}  // namespace model
