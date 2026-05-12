#pragma once

#include <algorithm>
#include <cstring>
#include <vector>

#include "core/model/types/type.h"

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
        assert(src != nullptr);
        bool value;
        std::memcpy(&value, src, sizeof(bool));
        return value ? "true" : "false";
    }

    void ValueFromStr(std::byte* dst, std::string s) const override {
        assert(dst != nullptr);
        s.erase(0, s.find_first_not_of(" \t\n\r"));
        s.erase(s.find_last_not_of(" \t\n\r") + 1);
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);

        static std::vector<std::string> const kTrueValues = {"true", "t", "1"};
        static std::vector<std::string> const kFalseValues = {"false", "f", "0"};

        bool value;
        if (std::find(kTrueValues.begin(), kTrueValues.end(), s) != kTrueValues.end()) {
            value = true;
        } else if (std::find(kFalseValues.begin(), kFalseValues.end(), s) != kFalseValues.end()) {
            value = false;
        } else {
            throw std::invalid_argument("Invalid bool literal: " + s);
        }
        std::memcpy(dst, &value, sizeof(bool));
    }

    CompareResult Compare(std::byte const* l, std::byte const* r) const override {
        assert(l != nullptr and r != nullptr);
        bool lv, rv;
        std::memcpy(&lv, l, sizeof(bool));
        std::memcpy(&rv, r, sizeof(bool));

        if (lv == rv) return CompareResult::kEqual;
        return lv ? CompareResult::kGreater : CompareResult::kLess;
    }

    size_t Hash(std::byte const* value) const override {
        bool v;
        std::memcpy(&v, value, sizeof(v));
        return std::hash<bool>{}(v);
    }
};

}  // namespace model
