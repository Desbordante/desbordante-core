#pragma once

#include <utility>

namespace algos::hymde::utility {
template <auto v>
struct CompileTimeValue {
    using Type = decltype(v);

    constexpr static bool kCompileTime = true;
    constexpr static Type value = v;
};

template <typename Value>
struct NonCompileTimeValue {
    using Type = Value;

    static constexpr bool kCompileTime = false;
    Value value;

    NonCompileTimeValue(Value value) : value(std::move(value)) {}
};

template <auto v>
struct CompileTimeOptionalLike {
    using Type = decltype(v);

    struct OptionalLike {
        static constexpr bool has_value() noexcept {
            return true;
        }

        constexpr Type operator*() const noexcept {
            return v;
        }
    };

    constexpr static bool kCompileTime = true;
    constexpr static OptionalLike value{};
};
}  // namespace algos::hymde::utility
