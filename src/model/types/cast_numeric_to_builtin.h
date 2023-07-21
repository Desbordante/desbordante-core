#pragma once
#include "types.h"
#include "builtin.h"
namespace model{

struct ICastToCppType{
    virtual model::Double get_double(std::byte * buf) = 0;
    virtual float get_float(std::byte * buf)=0;
    virtual model::Int get_int(std::byte * buf)=0;
    template <typename return_type>
    return_type unsafe_cast_to_cpp(model::TypeId inp_type, std::byte* buf){
        switch (inp_type)
        {
            case model::TypeId::kDouble:
            {
                static_assert(std::is_same<return_type,Double>::value,"can only convert model::DoubleType to Double");
                return *reinterpret_cast<Double*>(buf); 
            }    
            break;

            default:
                static_assert(true,"undefined type conversion");
                break;
        }
    }
};

struct CastToDoubleType : ICastToCppType{
    model::Double get_double(std::byte * buf) override{
        return *reinterpret_cast<Double*>(buf);
    }
    model::Int get_int(std::byte * buf)override{
        return static_cast<model::Int> (*reinterpret_cast<Double*>(buf));
    }
    float get_float(std::byte * buf)override{
        return static_cast<float> (*reinterpret_cast<Double*>(buf));
    }
};
}//namespace model