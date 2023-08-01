#pragma once
#include "cast_to_builtin_type.h"
namespace model {
class CastFromIntType:public CastToCppType<Int> {};
class CastFromIntTypeToNumeric : public ICastToNumericType {
public:
    std::byte* CastToDoubleType(const std::byte* data) override {
        return this->MakeValue<Double>(static_cast<Double>(*reinterpret_cast<const Int*>(data)));
    }
    std::byte* CastToIntType(const std::byte* data) override {
        return this->MakeValue<Int>(static_cast<Int>(*reinterpret_cast<const Int*>(data)));
    }
};
} //namespace model