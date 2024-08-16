#pragma once

#include <cstddef>
#include <functional>
#include <tuple>

namespace util {
template <std::size_t N, typename R, typename... Args>
auto GetParam(std::function<R(Args...)>) -> std::tuple_element_t<N, std::tuple<Args...>>;

template <typename Func, std::size_t N>
using ArgumentType =
        decltype(GetParam<N>(std::declval<decltype(std::function{std::declval<Func>()})>()));
}  // namespace util
