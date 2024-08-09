#ifndef COMPONENT_H
#define COMPONENT_H

#include <string>

#include <easylogging++.h>

#include "type.h"

enum ValType { kMinusInf, kFinite, kPlusInf };

// describes a component of a point
class Component {
private:
    std::byte const* value_;
    model::Type const* type_;
    ValType val_type_;

public:
    Component() noexcept : value_(nullptr), type_(nullptr), val_type_(kFinite){};

    Component(std::byte const* value, model::Type const* type, ValType val_type = kFinite) noexcept
        : value_(value), type_(type), val_type_(val_type){};

    Component(Component const& comp) noexcept = default;

    Component(Component&& comp) noexcept = default;

    Component& operator=(Component const& comp) noexcept = default;

    Component& operator=(Component&& comp) noexcept = default;

    std::string ToString() const {
        if (val_type_ == kPlusInf) return "+Inf";
        if (val_type_ == kMinusInf) return "-Inf";

        model::TypeId type_id = type_->GetTypeId();
        switch (type_id) {
            case model::TypeId::kInt:
                return std::to_string(model::Type::GetValue<model::Int>(value_));
                break;
            case model::TypeId::kDouble:
                return std::to_string(model::Type::GetValue<model::Double>(value_));
                break;
            case model::TypeId::kString:
                return model::Type::GetValue<model::String>(value_);
                break;
            default:
                assert(false);
                __builtin_unreachable();
        }
    }

    bool operator<(Component const& comp) const {
        assert(type_->GetTypeId() == comp.type_->GetTypeId());

        if (comp.val_type_ == kFinite) {
            if (val_type_ == kMinusInf) return true;
            if (val_type_ == kPlusInf) return false;
        }

        if (val_type_ == kFinite) {
            if (comp.val_type_ == kPlusInf) return true;
            if (comp.val_type_ == kMinusInf) return false;
        }

        return type_->Compare(value_, comp.value_) == model::CompareResult::kLess;
    }

    bool operator<=(Component const& comp) const {
        if (*this < comp or type_->Compare(value_, comp.value_) == model::CompareResult::kEqual)
            return true;

        return false;
    }

    bool operator>(Component const& comp) const {
        if (!(*this <= comp)) return true;

        return false;
    }

    bool operator>=(Component const& comp) const {
        if (!(*this < comp)) return true;

        return false;
    }

    bool operator==(Component const& comp) const {
        return *this <= comp and *this >= comp;
    }

    void Swap(Component& comp) {
        std::swap(value_, comp.value_);
        std::swap(type_, comp.type_);
        std::swap(val_type_, comp.val_type_);
    }

    ValType& GetValType() {
        return val_type_;
    }

    model::Type const* GetType() const {
        return type_;
    }

    std::byte const* GetVal() {
        return value_;
    }
};

#endif  // COMPONENT_H