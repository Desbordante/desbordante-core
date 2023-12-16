#pragma once

#include <chrono>
#include <functional>
#include <utility>

namespace util {
/// invoke and return the time taken for execution
template <typename... Ts>
size_t TimedInvoke(Ts&&... invoke_args) {
    auto const start = std::chrono::system_clock::now();
    std::invoke(std::forward<Ts>(invoke_args)...);
    auto const end = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}
}  // namespace util
