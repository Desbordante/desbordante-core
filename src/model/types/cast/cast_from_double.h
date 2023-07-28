#pragma once
#include "cast_to_builtin_type.h"
#include "icast_to_numeric_type.h"
namespace model{
class CastFromDoubleType :public CastToCppType<Double>{};
class CastFromDoubleTypeToNumeric: public ICastToNumericType{
    public:
    std::byte* CastToDoubleType(const std::byte* data) override{
        //model::DoubleType type = model::DoubleType();
        return this->MakeValue<Double>(*reinterpret_cast<const Double*> (data));
    }
    std::byte* CastToIntType(const std::byte* data) override{
        return this->MakeValue<Int>(static_cast<Int>(*reinterpret_cast<const Double*> (data)));
    }
};
}//namespace model