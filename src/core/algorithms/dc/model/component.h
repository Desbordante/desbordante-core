#pragma once

#include <string>

#include "operator.h"
#include "types.h"

namespace algos {

namespace dc {

enum class ValType { kMinusInf, kFinite, kPlusInf };

//  @brief Component of a k-dimensional point
class Component {
private:
    std::byte const* val_;
    model::Type const* type_;
    ValType val_type_;

    bool CompareNumeric(std::byte const* l_val, model::Type const* lhs_type, std::byte const* r_val,
                        model::Type const* rhs_type, model::CompareResult res) const;

public:
    Component() noexcept : val_(nullptr), type_(nullptr), val_type_(ValType::kFinite) {};

    Component(std::byte const* value, model::Type const* type,
              ValType val_type = ValType::kFinite) noexcept
        : val_(value), type_(type), val_type_(val_type) {};

    std::string ToString() const;

    static bool Eval(Component const& lhs, Component const& rhs, Operator const& op);

    bool operator<(Component const& rhs) const;

    bool operator==(Component const& rhs) const;

    bool operator<=(Component const& rhs) const {
        return *this < rhs or *this == rhs;
    }

    bool operator>(Component const& rhs) const {
        return !(*this <= rhs);
    }

    bool operator>=(Component const& rhs) const {
        return !(*this < rhs);
    }

    void Swap(Component& rhs) {
        std::swap(val_, rhs.val_);
        std::swap(type_, rhs.type_);
        std::swap(val_type_, rhs.val_type_);
    }

    ValType& GetValType() {
        return val_type_;
    }

    model::Type const* GetType() const {
        return type_;
    }

    std::byte const* GetVal() {
        return val_;
    }
};

}  // namespace dc

}  // namespace algos
