#pragma once

#include <algorithm>
#include <cstring>
#include <vector>

#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"

namespace model {

class BoolType final : public IMetrizableType {
public:
    BoolType() noexcept : IMetrizableType(TypeId::kBool) {}

    std::unique_ptr<Type> CloneType() const override {
        return std::make_unique<BoolType>();
    }

    size_t GetSize() const override {
        return sizeof(Bool);
    }

    std::string ValueToString(std::byte const* src) const override {
        assert(src != nullptr);
        Bool value;
        std::memcpy(&value, src, sizeof(Bool));
        return value ? "true" : "false";
    }

    void ValueFromStr(std::byte* dst, std::string s) const override {
        assert(dst != nullptr);

        Bool value = TypeConverter<Bool>::kConvert(s);
        std::memcpy(dst, &value, sizeof(Bool));
    }

    CompareResult Compare(std::byte const* l, std::byte const* r) const override {
        assert(l != nullptr and r != nullptr);
        Bool lv, rv;
        std::memcpy(&lv, l, sizeof(Bool));
        std::memcpy(&rv, r, sizeof(Bool));

        if (lv == rv) return CompareResult::kEqual;
        return lv ? CompareResult::kGreater : CompareResult::kLess;
    }

    size_t Hash(std::byte const* value) const override {
        Bool v;
        std::memcpy(&v, value, sizeof(v));
        return std::hash<Bool>{}(v);
    }

    double Dist(std::byte const* l, std::byte const* r) const override {
        return (GetValue<Bool>(l) == GetValue<Bool>(r)) ? 0 : 1;
    }
};

}  // namespace model
