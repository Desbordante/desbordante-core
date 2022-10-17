#pragma once

#include "string_type.h"

namespace model {

class BigIntType : public StringType {
public:
    BigIntType() : StringType(TypeId::kBigInt){};
};

}  // namespace model
