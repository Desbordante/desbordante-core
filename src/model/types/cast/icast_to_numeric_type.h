#pragma once
namespace model{
class ICastToNumericType{
public:
    virtual std::byte* CastToDoubleType(const std::byte* data) = 0;
    virtual std::byte* CastToIntType(const std::byte* data) = 0;
protected:
    template <typename T>
    std::byte* MakeValue(T const literal) const {
        auto* buf = new std::byte[sizeof(T)];
        *reinterpret_cast<T*>(buf) = literal;
        return buf;
    }
};
} //namespace model