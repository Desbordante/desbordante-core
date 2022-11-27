#pragma once

#include <functional>
#include <string_view>

namespace algos::config {
using OptAddFunc = std::function<void(IOption *, std::vector<std::string_view> const &)>;
}  // namespace algos::config
