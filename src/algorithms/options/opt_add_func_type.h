#pragma once

#include <functional>
#include <string_view>

namespace algos::config {
using OptAddFunc = std::function<void(std::string_view, std::vector<std::string_view> const &)>;
}  // namespace algos::config
