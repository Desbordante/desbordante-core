#pragma once

#include "numeric_type.h"

namespace model {

class IntType final : public NumericType<Int> {
public:
    IntType() noexcept : NumericType<Int>(TypeId::kInt) {}

    [[nodiscard]] std::unique_ptr<Type> CloneType() const {
        return std::make_unique<IntType>();
    }
};

}  // namespace model
