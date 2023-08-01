#pragma once
#include "icast_to_builtin_type.h"
namespace model {
template <typename T>
class CastToCppType : public ICastToCppType {
public:
    model::Double GetDouble(std::byte* buf) override {
        return static_cast<model::Double>(*reinterpret_cast<T*>(buf));
    }
    model::Int GetInt(std::byte* buf) override {
        return static_cast<model::Int>(*reinterpret_cast<T*>(buf));
    }
    float GetFloat(std::byte* buf) override {
        return static_cast<float>(*reinterpret_cast<T*>(buf));
    }
};
}  // namespace model