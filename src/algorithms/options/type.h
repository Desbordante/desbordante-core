#pragma once

#include <optional>

#include "algorithms/options/option.h"

namespace algos::config {

template<typename T>
struct OptionType {
    explicit OptionType(OptionInfo const info, std::optional<T> default_value = {},
                        typename Option<T>::NormalizeFunc normalize = {})
            : info_(info), default_value_(std::move(default_value)),
              normalize_(std::move(normalize)) {}

    [[nodiscard]] Option<T> GetOption(T *value_ptr) const {
        return {info_, value_ptr, normalize_, default_value_};
    }

    [[nodiscard]] std::string_view GetName() const {
        return info_.GetName();
    }

private:
    OptionInfo const info_;
    std::optional<T> const default_value_;
    std::function<void(T &)> const normalize_;
};

template<typename... Types>
std::vector<std::string_view> GetOptionNames(OptionType<Types> const &... options) {
    std::vector<std::string_view> names{};
    (names.emplace_back(options.GetName()), ...);
    return names;
}

}
