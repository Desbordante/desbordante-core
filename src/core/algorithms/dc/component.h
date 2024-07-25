#ifndef COMPONENT_H
#define COMPONENT_H

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
    Component() : value_(nullptr), type_(nullptr), val_type_(kFinite) {};

    Component(std::byte const* value, model::Type const* type, ValType val_type = kFinite)
        : value_(value), type_(type), val_type_(val_type) {}

    Component(Component const& comp) {
        value_ = comp.value_;
        type_ = comp.type_;
    };

    Component(Component&& comp) {
        value_ = comp.value_;
        type_ = comp.type_;
    };

    Component& operator=(Component const& comp) {
        value_ = comp.value_;
        type_ = comp.type_;
        return *this;
    };

    Component& operator=(Component&& comp) = default;

    bool operator<(Component const& comp) const {
        LOG(INFO) << comp.GetType()->GetTypeId();
        LOG(INFO) << type_->GetTypeId();
        assert(type_->GetTypeId() == comp.type_->GetTypeId());

        if (val_type_ == kMinusInf or comp.val_type_ == kPlusInf or
            type_->Compare(value_, comp.value_) == model::CompareResult::kLess)
            return true;

        return false;
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

    ValType& GetValType() {
        return val_type_;
    }

    model::Type const* GetType() const {
        return type_;
    }

    const std::byte* GetVal() {
        return value_;
    }
};

#endif  // COMPONENT_H