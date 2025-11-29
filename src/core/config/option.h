#pragma once

#include <cassert>
#include <functional>
#include <typeindex>
#include <typeinfo>

#include <boost/any.hpp>

#include "core/config/exceptions.h"
#include "core/config/ioption.h"

namespace config {

// This class is responsible for configuration values. It is provided with a
// pointer to a field of an algorithm object, option name, and option
// description.
// It can also normalize the provided value and check it for correctness.
// Some options may have a default value, which can be provided as well.
// However, some defaults may depend on internal state of an algorithms, so a
// function that returns that default value may be passed instead.
template <typename T>
class Option : public IOption {
public:
    using DefaultFunc = std::function<T()>;
    using ValueCheckFunc = std::function<void(T const &)>;
    using IsRequiredFunc = std::function<bool()>;
    using CondCheckFunc = std::function<bool(T const &val)>;
    using OptCondVector = std::vector<std::pair<CondCheckFunc, std::vector<std::string_view>>>;
    using NormalizeFunc = std::function<void(T &)>;

    Option(T *value_ptr, std::string_view name, std::string_view description,
           DefaultFunc default_func = nullptr)
        : value_ptr_(value_ptr),
          name_(name),
          description_(description),
          default_func_(std::move(default_func)) {}

    Option(T *value_ptr, std::string_view name, std::string_view description, T default_value)
        : Option(value_ptr, name, description,
                 [default_value = std::move(default_value)]() { return default_value; }) {}

    std::vector<std::string_view> Set(boost::any const &value) override;

    void Unset() override {
        is_set_ = false;
    }

    [[nodiscard]] std::string_view GetName() const override {
        return name_;
    }

    [[nodiscard]] std::string_view GetDescription() const override {
        return description_;
    }

    [[nodiscard]] std::type_index GetTypeIndex() const override {
        return typeid(T);
    }

    [[nodiscard]] bool IsSet() const override {
        return is_set_;
    }

    Option &SetValueCheck(ValueCheckFunc value_check) {
        assert(!value_check_);
        value_check_ = std::move(value_check);
        return *this;
    }

    // Some options may become required depending on this option's value and/or
    // their correctness may depend on the value. If that is the case, a vector
    // of {predicate, option names} pairs may be provided to this method.
    // If after a predicate in one of the pairs holds for the provided value,
    // then the corresponding option names will be returned, which will then
    // become required options of the algorithm.
    // Order matters: predicates are checked first-to-last, and `Set` returns
    // only the names of the first pair where the predicate holds. An empty
    // predicate is equivalent to an always-true predicate, and thus must
    // always be last.
    Option &SetConditionalOpts(OptCondVector opt_cond) {
        assert(opt_cond_.empty());
        assert(!opt_cond.empty());
        opt_cond_ = std::move(opt_cond);
        return *this;
    }

    Option &SetNormalizeFunc(NormalizeFunc normalize_func) {
        assert(!normalize_func_);
        normalize_func_ = std::move(normalize_func);
        return *this;
    }

    Option &SetIsRequiredFunc(IsRequiredFunc is_required) {
        assert(!is_required_);
        is_required_ = std::move(is_required);
        return *this;
    }

    OptValue GetOptValue() const override {
        return OptValue{std::type_index(typeid(T)), boost::any(*value_ptr_)};
    }

    bool IsRequired() const noexcept override {
        return is_required_ == nullptr || is_required_();
    }

private:
    T ConvertValue(boost::any const &value) const;

    bool is_set_ = false;
    T *value_ptr_;
    std::string_view name_;
    std::string_view description_;
    DefaultFunc default_func_;
    ValueCheckFunc value_check_{};
    IsRequiredFunc is_required_{};
    OptCondVector opt_cond_{};
    NormalizeFunc normalize_func_{};
};

template <typename T>
std::vector<std::string_view> Option<T>::Set(boost::any const &value) {
    assert(!is_set_);
    T converted_value = ConvertValue(value);
    if (normalize_func_) normalize_func_(converted_value);
    if (value_check_) value_check_(converted_value);

    assert(value_ptr_ != nullptr);
    is_set_ = true;
    std::vector<std::string_view> new_options{};
    for (auto const &[cond, opts] : opt_cond_) {
        if (!cond || cond(converted_value)) {
            new_options = opts;
            break;
        }
    }
    *value_ptr_ = std::move(converted_value);
    return new_options;
}

template <typename T>
T Option<T>::ConvertValue(boost::any const &value) const {
    std::string const no_value_no_default =
            std::string("No value was provided to an option without a default value (") +
            GetName().data() + ")";
    if (value.empty()) {
        if (!default_func_) throw ConfigurationError(no_value_no_default);
        return default_func_();
    } else {
        if (value.type() != typeid(T))
            throw ConfigurationError(std::string("Incorrect type for option ") + name_.data());
        return boost::any_cast<T>(value);
    }
}

}  // namespace config
