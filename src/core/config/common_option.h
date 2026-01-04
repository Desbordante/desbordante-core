#pragma once

#include <optional>
#include <string_view>
#include <variant>

#include <boost/functional/overloaded_function.hpp>

#include "core/config/option.h"

namespace config {

// Aids in creating options that come up often in unrelated algorithms, like
// null equality.
template <typename T>
class CommonOption {
private:
    using DefaultFuncType = typename Option<T>::DefaultFunc;

    std::string_view const name_;
    std::string_view const description_;
    std::variant<std::optional<T>, DefaultFuncType> const default_init_v_;
    typename Option<T>::NormalizeFunc const normalize_func_;
    typename Option<T>::ValueCheckFunc const value_check_func_;

public:
    CommonOption(std::string_view name, std::string_view description,
                 std::variant<std::optional<T>, DefaultFuncType> default_init_v = std::nullopt,
                 typename Option<T>::NormalizeFunc normalize_func = nullptr,
                 typename Option<T>::ValueCheckFunc value_check_func = nullptr)
        : name_(name),
          description_(description),
          default_init_v_(default_init_v),
          normalize_func_(normalize_func),
          value_check_func_(value_check_func) {}

    [[nodiscard]] Option<T> operator()(T* value_ptr) const {
        auto from_default_func = [value_ptr, this](DefaultFuncType default_func) {
            return Option{value_ptr, name_, description_, default_func};
        };
        auto from_default_value = [value_ptr, this](std::optional<T> const& default_value) {
            return default_value.has_value()
                           ? Option{value_ptr, name_, description_, default_value.value()}
                           : Option{value_ptr, name_, description_};
        };
        Option<T> option =
                std::visit(boost::make_overloaded_function(from_default_func, from_default_value),
                           default_init_v_);
        if (normalize_func_) option.SetNormalizeFunc(normalize_func_);
        if (value_check_func_) option.SetValueCheck(value_check_func_);
        return option;
    }

    [[nodiscard]] std::string_view GetName() const {
        return name_;
    }
};

}  // namespace config
