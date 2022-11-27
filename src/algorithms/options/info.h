#pragma once

#include <string_view>

namespace algos::config {

struct OptionInfo {
    OptionInfo(std::string_view name, std::string_view description);

    [[nodiscard]] std::string_view GetName() const;
    [[nodiscard]] std::string_view GetDescription() const;

private:
    std::string_view name_;
    std::string_view description_;
};

}  // namespace algos::config
