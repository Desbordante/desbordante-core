#pragma once

#include <cstddef>
#include <functional>
#include <tuple>

namespace util {
namespace detail {
template <std::size_t N, typename R, typename... Args>
std::tuple_element_t<N, std::tuple<Args...>> GetParam(std::function<R(Args...)>&&) {}
}  // namespace detail

template <typename Func, std::size_t N>
using ArgumentType = decltype(detail::GetParam<N>(std::function(std::declval<Func>())));
}  // namespace util
