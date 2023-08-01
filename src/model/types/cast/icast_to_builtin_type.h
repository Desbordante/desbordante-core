#pragma once
#include "model/types/builtin.h"
namespace model {
class ICastToCppType {
public:
    virtual model::Double GetDouble(std::byte* buf) = 0;
    virtual float GetFloat(std::byte* buf) = 0;
    virtual model::Int GetInt(std::byte* buf)= 0;
    template <typename return_type>
    return_type UnsafeCastToCpp(model::TypeId inp_type, std::byte* buf) {
        switch (inp_type) {
            case model::TypeId::kDouble: {
                static_assert(std::is_same<return_type,Double>::value, 
                              "can only convert model::DoubleType to Double");
                return *reinterpret_cast<Double*>(buf); 
            } break;
            case model::TypeId::kInt:{
                static_assert(std::is_same<return_type,Int>::value,
                              "can only convert model::IntType to Int");
                return *reinterpret_cast<model::Int*>(buf);
            }
            default:
                static_assert(true, "undefined type conversion");
                break;
        }
    }
};
} // namespace model