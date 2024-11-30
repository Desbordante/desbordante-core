#pragma once

#include <cstddef>
#include <functional>
#include <tuple>

namespace util {
namespace detail {
template <std::size_t N, typename R, typename... Args>
std::tuple_element_t<N, std::tuple<Args...>> GetParam(std::function<R(Args...)>&&) {}

// GCC 11.4.0 workaround
template <typename T>
auto GetFunction(T&& obj) {
    return std::function{obj};
}
}  // namespace detail

template <typename Func, std::size_t N>
using ArgumentType = decltype(detail::GetParam<N>(detail::GetFunction(std::declval<Func>())));
}  // namespace util
