#pragma once

#include <utility>

namespace algos::hymde::utility {
template <auto V>
struct CompileTimeValue {
    using Type = decltype(V);

    constexpr static bool kCompileTime = true;
    constexpr static Type value = V;
};

template <typename Value>
struct NonCompileTimeValue {
    using Type = Value;

    static constexpr bool kCompileTime = false;
    Value value;

    NonCompileTimeValue(Value value) : value(std::move(value)) {}
};

template <auto V>
struct CompileTimeOptionalLike {
    using Type = decltype(V);

    struct OptionalLike {
        static constexpr bool has_value() noexcept {
            return true;
        }

        constexpr Type operator*() const noexcept {
            return V;
        }
    };

    constexpr static bool kCompileTime = true;
    constexpr static OptionalLike value{};
};
}  // namespace algos::hymde::utility
