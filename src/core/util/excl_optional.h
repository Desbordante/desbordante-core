#pragma once

#include <optional>

#include "util/construct.h"

namespace util {
template <typename T, bool (*HasValueCheck)(T const&), T (*GetDefault)() = Construct<T>>
class ExclOptional {
    T value_ = GetDefault();

public:
    ExclOptional() = default;

    ExclOptional(std::nullopt_t) {}

    template <typename... Args>
    ExclOptional(Args&&... args) : value_(std::forward<Args>(args)...) {}

    T& operator*() noexcept {
        return value_;
    }

    T const& operator*() const noexcept {
        return value_;
    }

    // Even if HasValue returns false, value may be usable.
    T* operator->() noexcept {
        return &value_;
    }

    T const* operator->() const noexcept {
        return &value_;
    }

    bool HasValue() const noexcept {
        return HasValueCheck(value_);
    }
};
}  // namespace util
