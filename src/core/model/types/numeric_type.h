#pragma once

#include <cassert>
#include <cmath>
#include <limits>
#include <sstream>

#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"

namespace model {

class INumericType : public IMetrizableType {
public:
    using NumericBinop = std::byte* (INumericType::*)(std::byte const*, std::byte const*,
                                                      std::byte*) const;

    explicit INumericType(TypeId id) noexcept : IMetrizableType(id) {}

    virtual void CastTo(std::byte* value, TypeId to_type) const = 0;

    void CastTo(std::byte* value, INumericType const& to) const {
        CastTo(value, to.GetTypeId());
    }

    template <typename T>
    T GetValueAs(std::byte const* value) const;

    virtual CompareResult CompareNumeric(std::byte const* l, INumericType const* l_type,
                                         std::byte const* r, INumericType const* r_type) const = 0;

    virtual bool EvalComparison(std::byte const* l_val, INumericType const* lhs_type,
                                std::byte const* r_val, INumericType const* rhs_type,
                                CompareResult res) const = 0;

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
            assert(false);
            __builtin_unreachable();
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

    void ValueFromStr(std::byte* buf, std::string s) const override;
    CompareResult Compare(std::byte const* l, std::byte const* r) const override;
    CompareResult CompareNumeric(std::byte const* l, INumericType const* l_type, std::byte const* r,
                                 INumericType const* r_type) const override;
    bool EvalComparison(std::byte const* l_val, INumericType const* lhs_type,
                        std::byte const* r_val, INumericType const* rhs_type,
                        CompareResult res) const override;

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

    void CastTo(std::byte* value, TypeId to_type) const override;
};

template <typename T>
void NumericType<T>::CastTo(std::byte* value, TypeId to_type) const {
    switch (to_type) {
        case TypeId::kDouble: {
            model::Double data = GetValueAs<model::Double>(value);
            INumericType::GetValue<model::Double>(value) = data;
            break;
        }
        case TypeId::kInt: {
            model::Int data = GetValueAs<model::Int>(value);
            INumericType::GetValue<model::Int>(value) = data;
            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

template <typename T>
bool NumericType<T>::EvalComparison(std::byte const* l_val, INumericType const* l_type,
                                    std::byte const* r_val, INumericType const* r_type,
                                    CompareResult res) const {
    return l_type->CompareNumeric(l_val, l_type, r_val, r_type) == res;
}

template <typename T>
CompareResult NumericType<T>::CompareNumeric(std::byte const* l, INumericType const* l_type,
                                             std::byte const* r, INumericType const* r_type) const {
    Double l_val = l_type->GetValueAs<Double>(l);
    Double r_val = r_type->GetValueAs<Double>(r);
    if (l_val == r_val) {
        return CompareResult::kEqual;
    }
    if (l_val < r_val) {
        return CompareResult::kLess;
    }
    return CompareResult::kGreater;
}

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
    GetValue(buf) = TypeConverter<T>::kConvert(s);
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
