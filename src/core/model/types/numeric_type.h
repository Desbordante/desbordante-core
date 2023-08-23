#pragma once

#include <cassert>
#include <cmath>
#include <limits>
#include <sstream>

#include "imetrizable_type.h"
#include "type.h"

namespace model {

class INumericType : public IMetrizableType {
public:
    using NumericBinop = std::byte* (INumericType::*)(std::byte const*, std::byte const*, std::byte*) const;

    explicit INumericType(TypeId id) noexcept : IMetrizableType(id) {}

    void CastTo(std::byte* value, TypeId to_type) const;
    void CastTo(std::byte* value, INumericType const& to) const;

    template <typename T>
    T GetValueAs(std::byte const* value) const;

    virtual std::byte* Negate(std::byte const* value, std::byte* res) const = 0;
    virtual std::byte* Add(std::byte const* l, std::byte const* r, std::byte* res) const = 0;
    virtual std::byte* Sub(std::byte const* l, std::byte const* r, std::byte* res) const = 0;
    virtual std::byte* Mul(std::byte const* l, std::byte const* r, std::byte* res) const = 0;
    virtual std::byte* Div(std::byte const* l, std::byte const* r, std::byte* res) const = 0;
    virtual std::byte* Min(std::byte const* l, std::byte const* r, std::byte* res) const = 0;
    virtual std::byte* Max(std::byte const* l, std::byte const* r, std::byte* res) const = 0;
    virtual std::byte* Power(std::byte const* num, long double pow, std::byte* res) const = 0;
    virtual std::byte* Abs(std::byte const* num, std::byte* res) const = 0;

    virtual std::byte* MakeValueOfInt(int value) const = 0;

    [[nodiscard]] virtual std::byte const* Min() const = 0;
    [[nodiscard]] virtual std::byte const* Max() const = 0;
};

template <typename T>
T INumericType::GetValueAs(std::byte const* value) const {
    switch (GetTypeId()) {
        case TypeId::kDouble: {
            model::Double data = INumericType::GetValue<Double>(value);
            return static_cast<T>(data);
        }
        case TypeId::kInt: {
            model::Int data = INumericType::GetValue<Int>(value);
            return static_cast<T>(data);
        }
        default: {
            assert(true);
            return T();
        }
    }
}

inline void INumericType::CastTo(std::byte* value, INumericType const& to) const {
    this->CastTo(value, to.GetTypeId());
}

inline void INumericType::CastTo(std::byte* value, TypeId to_type) const {
    if (value == nullptr) {
        assert(true);
    }
    switch (GetTypeId()) {
        case TypeId::kDouble: {
            switch (to_type) {
                case TypeId::kInt: {
                    model::Int data = INumericType::GetValueAs<model::Int>(value);
                    INumericType::GetValue<model::Int>(value) = data;
                    break;
                }
                case TypeId::kDouble: {
                    break;
                }
                default: {
                    assert(true);
                    break;
                }
            }
            break;
        }
        case TypeId::kInt: {
            switch (to_type) {
                case TypeId::kDouble: {
                    model::Double data = INumericType::GetValueAs<model::Double>(value);
                    INumericType::GetValue<model::Double>(value) = data;
                    break;
                }
                case TypeId::kInt: {
                    break;
                }
                default: {
                    assert(true);
                    break;
                }
            }
            break;
        }
        default: {
            assert(true);
            break;
        }
    }
}

template <typename T>
class NumericType : public INumericType {
protected:
    static T const& GetValue(std::byte const* buf) {
        return INumericType::GetValue<T>(buf);
    }
    static T& GetValue(std::byte* buf) {
        return INumericType::GetValue<T>(buf);
    }

public:
    using UnderlyingType = T;

    static constexpr T kMinValue = std::numeric_limits<T>::lowest();
    static constexpr T kMaxValue = std::numeric_limits<T>::max();

    explicit NumericType(TypeId id) : INumericType(id) {}

    CompareResult Compare(std::byte const* l, std::byte const* r) const override;
    void ValueFromStr(std::byte* buf, std::string s) const override;

    std::byte* Negate(std::byte const* value, std::byte* res) const override;
    std::byte* Add(std::byte const* l, std::byte const* r, std::byte* res) const override;
    std::byte* Sub(std::byte const* l, std::byte const* r, std::byte* res) const override;
    std::byte* Mul(std::byte const* l, std::byte const* r, std::byte* res) const override;
    std::byte* Div(std::byte const* l, std::byte const* r, std::byte* res) const override;
    std::byte* Min(std::byte const* l, std::byte const* r, std::byte* res) const override;
    std::byte* Max(std::byte const* l, std::byte const* r, std::byte* res) const override;
    std::byte* Power(std::byte const* num, long double pow, std::byte* res) const override;
    std::byte* Abs(std::byte const* num, std::byte* res) const override;

    std::byte* MakeValueOfInt(int value) const override {
        return MakeValue(value);
    }

    double Dist(std::byte const* l, std::byte const* r) const override {
        return std::abs(GetValue(l) - GetValue(r));
    }

    [[nodiscard]] std::byte const* Min() const override {
        return reinterpret_cast<std::byte const*>(&kMinValue);
    }

    [[nodiscard]] std::byte const* Max() const override {
        return reinterpret_cast<std::byte const*>(&kMaxValue);
    }

    std::string ValueToString(std::byte const* value) const override {
        return std::to_string(GetValue(value));
    }

    [[nodiscard]] size_t GetSize() const override {
        return sizeof(T);
    }

    size_t Hash(std::byte const* value) const override {
        return std::hash<T>{}(GetValue(value));
    }

    std::byte* MakeValue(T const literal) const {
        auto* buf = new std::byte[GetSize()];
        GetValue(buf) = literal;
        return buf;
    }
};

template <typename T>
CompareResult NumericType<T>::Compare(std::byte const* l, std::byte const* r) const {
    T l_val = GetValue(l);
    T r_val = GetValue(r);
    if (l_val == r_val) {
        return CompareResult::kEqual;
    }
    if (l_val < r_val) {
        return CompareResult::kLess;
    }
    return CompareResult::kGreater;
}

template <typename T>
void NumericType<T>::ValueFromStr(std::byte* buf, std::string s) const {
    GetValue(buf) = TypeConverter<T>::convert(s);
}

template <typename T>
std::byte* NumericType<T>::Negate(std::byte const* value, std::byte* res) const {
    GetValue(res) = -GetValue(value);
    return res;
}
template <typename T>
std::byte* NumericType<T>::Add(std::byte const* l, std::byte const* r, std::byte* res) const {
    GetValue(res) = GetValue(l) + GetValue(r);
    return res;
}

template <typename T>
std::byte* NumericType<T>::Sub(std::byte const* l, std::byte const* r, std::byte* res) const {
    GetValue(res) = GetValue(l) - GetValue(r);
    return res;
}

template <typename T>
std::byte* NumericType<T>::Mul(std::byte const* l, std::byte const* r, std::byte* res) const {
    GetValue(res) = GetValue(l) * GetValue(r);
    return res;
}

template <typename T>
std::byte* NumericType<T>::Div(std::byte const* l, std::byte const* r, std::byte* res) const {
    GetValue(res) = GetValue(l) / GetValue(r);
    return res;
}

template <typename T>
std::byte* NumericType<T>::Min(std::byte const* l, std::byte const* r, std::byte* res) const {
    GetValue(res) = std::min(GetValue(l), GetValue(r));
    return res;
}

template <typename T>
std::byte* NumericType<T>::Max(std::byte const* l, std::byte const* r, std::byte* res) const {
    GetValue(res) = std::max(GetValue(l), GetValue(r));
    return res;
}

template <typename T>
std::byte* NumericType<T>::Power(std::byte const* num, long double pow, std::byte* res) const {
    GetValue(res) = std::pow(GetValue(num), pow);
    return res;
}

template <typename T>
std::byte* NumericType<T>::Abs(std::byte const* num, std::byte* res) const {
    GetValue(res) = std::abs(GetValue(num));
    return res;
}

}  // namespace model
