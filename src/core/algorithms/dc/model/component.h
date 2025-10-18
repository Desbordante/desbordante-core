#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include "algorithms/dc/model/operator.h"
#include "model/types/builtin.h"
#include "model/types/types.h"
#include "type.h"

namespace algos::dc {

enum class ValType { kMinusInf, kFinite, kPlusInf };

// @brief Component of a k-dimensional point
class Component {
private:
    std::byte const* val_;
    model::Type const* type_;
    ValType val_type_;

public:
    Component() noexcept : val_(nullptr), type_(nullptr), val_type_(ValType::kFinite) {};

    Component(std::byte const* value, model::Type const* type,
              ValType val_type = ValType::kFinite) noexcept
        : val_(value), type_(type), val_type_(val_type) {};

    std::string ToString() const;

    static bool Eval(Component const& lhs, Component const& rhs, Operator const& op);

    auto operator<=>(Component const&) const = default;

    bool operator<(Component const& rhs) const;

    bool operator==(Component const& rhs) const;

    bool operator!=(Component const& rhs) const {
        return !(*this == rhs);
    }

    bool operator<=(Component const& rhs) const {
        return *this < rhs or *this == rhs;
    }

    bool operator>(Component const& rhs) const {
        return !(*this <= rhs);
    }

    bool operator>=(Component const& rhs) const {
        return !(*this < rhs);
    }

    void Swap(Component& rhs) noexcept {
        std::swap(val_, rhs.val_);
        std::swap(type_, rhs.type_);
        std::swap(val_type_, rhs.val_type_);
    }

    ValType& GetValType() noexcept {
        return val_type_;
    }

    model::Type const* GetType() const noexcept {
        return type_;
    }

    std::byte const* GetVal() const noexcept {
        return val_;
    }

    class Hasher {
    public:
        size_t operator()(Component const& comp) const {
            return comp.type_->Hash(comp.val_);
        }
    };
};

}  // namespace algos::dc
