#pragma once

#include <optional>
#include <string_view>

#include "util/config/option.h"

namespace util::config {

// Aids in creating options that come up often in unrelated algorithms, like
// null equality.
template <typename T>
class CommonOption {
private:
    std::string_view const name_;
    std::string_view const description_;
    std::optional<T> const default_value_;
    typename Option<T>::NormalizeFunc const normalize_func_;
    typename Option<T>::ValueCheckFunc const value_check_func_;

public:
    CommonOption(std::string_view name, std::string_view description,
                 std::optional<T> default_value = std::nullopt,
                 typename Option<T>::NormalizeFunc normalize_func = nullptr,
                 typename Option<T>::ValueCheckFunc value_check_func = nullptr)
        : name_(name),
          description_(description),
          default_value_(default_value),
          normalize_func_(normalize_func),
          value_check_func_(value_check_func) {}

    [[nodiscard]] Option<T> operator()(T *value_ptr) const {
        auto option = default_value_.has_value()
                              ? Option{value_ptr, name_, description_, default_value_.value()}
                              : Option{value_ptr, name_, description_};
        if (normalize_func_) option.SetNormalizeFunc(normalize_func_);
        if (value_check_func_) option.SetValueCheck(value_check_func_);
        return option;
    }

    [[nodiscard]] std::string_view GetName() const {
        return name_;
    }
};

}  // namespace util::config
