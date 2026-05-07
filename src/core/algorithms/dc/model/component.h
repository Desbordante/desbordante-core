#pragma once

#include <cstddef>
#include <string>

#include "core/algorithms/dc/model/operator.h"
#include "core/model/types/builtin.h"
#include "core/model/types/types.h"

namespace algos::dc {

enum class ValType { kMinusInf, kFinite, kPlusInf };

// @brief Component of a k-dimensional point
class Component {
private:
    std::byte const* val_;
    model::Type const* type_;
    ValType val_type_;
    bool owns_;

public:
    Component() noexcept
        : val_(nullptr), type_(nullptr), val_type_(ValType::kFinite), owns_(false) {}

    Component(std::byte const* value, model::Type const* type, ValType val_type = ValType::kFinite,
              bool owns = false)
        : val_(owns && value != nullptr ? type->Clone(value) : value),
          type_(type),
          val_type_(val_type),
          owns_(owns && value != nullptr) {}

    // Convenience overload: Component(val, type, owns)
    Component(std::byte const* value, model::Type const* type, bool owns)
        : Component(value, type, ValType::kFinite, owns) {}

    Component(Component const& other)
        : val_(other.owns_ && other.val_ != nullptr ? other.type_->Clone(other.val_) : other.val_),
          type_(other.type_),
          val_type_(other.val_type_),
          owns_(other.owns_ && other.val_ != nullptr) {}

    Component(Component&& other) noexcept
        : val_(other.val_), type_(other.type_), val_type_(other.val_type_), owns_(other.owns_) {
        other.owns_ = false;
        other.val_ = nullptr;
    }

    Component& operator=(Component other) noexcept {
        Swap(other);
        return *this;
    }

    ~Component() {
        if (owns_) type_->Free(val_);
    }

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
        std::swap(owns_, rhs.owns_);
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
