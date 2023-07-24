#pragma once
//#include "types.h"
#include "builtin.h"
namespace model{

struct ICastToCppType{
    virtual model::Double GetDouble(std::byte * buf) = 0;
    virtual float GetFloat(std::byte * buf)=0;
    virtual model::Int GetInt(std::byte * buf)=0;
    template <typename return_type>
    return_type UnsafeCastToCpp(model::TypeId inp_type, std::byte* buf){
        switch (inp_type)
        {
            case model::TypeId::kDouble:
            {
                static_assert(std::is_same<return_type,Double>::value,"can only convert model::DoubleType to Double");
                return *reinterpret_cast<Double*>(buf); 
            }    
            break;
            case model::TypeId::kInt:{
                static_assert(std::is_same<return_type,Int>::value,"can only convert model::IntType to Int");
                return *reinterpret_cast<model::Int *>(buf);
            }
            default:
                static_assert(true,"undefined type conversion");
                break;
        }
    }
};
template <typename T>
class CastToCppType: public ICastToCppType{
    model::Double GetDouble(std::byte * buf) override{
        return static_cast<model::Double>(*reinterpret_cast<T*>(buf));
    }
    model::Int GetInt(std::byte * buf)override{
        return static_cast<model::Int> (*reinterpret_cast<T*>(buf));
    }
    float GetFloat(std::byte * buf)override{
        return static_cast<float> (*reinterpret_cast<T*>(buf));
    }
};
class CastFromDoubleType :public CastToCppType<Double>{};
class CastFromIntType:public CastToCppType<Int>{};
class EmptyCastToCpp:public ICastToCppType{
    model::Double GetDouble(std::byte * buf) override{
        throw std::logic_error("only unsafe cast implemented");
    }
    model::Int GetInt(std::byte * buf)override{
        throw std::logic_error("only unsafe cast implemented");
    }
    float GetFloat(std::byte * buf)override{
        throw std::logic_error("only unsafe cast implemented");
    }
};
}//namespace model