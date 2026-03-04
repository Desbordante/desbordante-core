#pragma once

#include "core/model/types/type.h"
#include <cstring>

namespace model {

class BoolType final : public Type {
public:
    BoolType() noexcept : Type(TypeId::kBool) {}

    std::unique_ptr<Type> CloneType() const override {
        return std::make_unique<BoolType>();
    }

    size_t GetSize() const override {
        return sizeof(bool);
    }

    std::string ValueToString(std::byte const* src) const override {
        bool value;
        std::memcpy(&value, src, sizeof(bool));
        return value ? "true" : "false";
    }

    void ValueFromStr(std::byte* dst, std::string s) const override {
        bool value = false;

        if (s == "true" || s == "TRUE" || s == "True") {
            value = true;
        } else if (s == "false" || s == "FALSE" || s == "False") {
            value = false;
        } else {
            throw std::invalid_argument("Invalid bool literal: " + s);
        }

        std::memcpy(dst, &value, sizeof(bool));
    }

    CompareResult Compare(std::byte const* l, std::byte const* r) const override {
        bool lv, rv;
        std::memcpy(&lv, l, sizeof(bool));
        std::memcpy(&rv, r, sizeof(bool));

        if (lv == rv) return CompareResult::kEqual;
        return lv ? CompareResult::kGreater : CompareResult::kLess;
    }

    size_t Hash(std::byte const* value) const override {
        bool v;
        std::memcpy(&v, value, sizeof(bool));
        return std::hash<bool>{}(v);
    }
};

} // namespace model