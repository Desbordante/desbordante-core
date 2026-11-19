#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include "core/model/types/type.h"

namespace pac::util {
/// @brief RAII wrapper around a value of arbitrary model::Type
/// Owns the value, doesn't own the type
class TypedValue {
private:
    std::byte const* value_;
    ::model::Type::Destructor destructor_;

    void Swap(TypedValue& other) noexcept {
        std::swap(value_, other.value_);
        std::swap(destructor_, other.destructor_);
    }

public:
    TypedValue(std::string const& str_value, ::model::Type const* type) {
        auto* ptr = type->Allocate();
        type->ValueFromStr(ptr, str_value);
        value_ = ptr;

        destructor_ = type->GetDestructor();
    }

    TypedValue() = delete;
    TypedValue(TypedValue const&) = delete;
    TypedValue& operator=(TypedValue const&) = delete;

    TypedValue(TypedValue&& other) noexcept
        : value_(std::exchange(other.value_, nullptr)),
          destructor_(std::exchange(other.destructor_, nullptr)) {}

    TypedValue& operator=(TypedValue&& other) noexcept {
        TypedValue tmp{std::move(other)};
        Swap(tmp);
        return *this;
    }

    ~TypedValue() {
        if (value_) {
            if (destructor_) {
                destructor_(value_);
            } else {
                delete[] value_;
            }
        }
    }

    std::byte const* Value() const noexcept {
        return value_;
    }
};
}  // namespace pac::util
