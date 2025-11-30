#pragma once

#include "core/model/types/string_type.h"

namespace model {

class BigIntType : public StringType {
public:
    BigIntType() : StringType(TypeId::kBigInt){};

    [[nodiscard]] std::unique_ptr<Type> CloneType() const override {
        return std::make_unique<BigIntType>();
    }
};

}  // namespace model
