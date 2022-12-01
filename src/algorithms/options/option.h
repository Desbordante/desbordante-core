#pragma once

#include <cassert>
#include <functional>
#include <optional>
#include <typeindex>
#include <typeinfo>

#include <boost/any.hpp>

#include "algorithms/options/info.h"
#include "algorithms/options/ioption.h"
#include "algorithms/options/opt_add_func_type.h"

namespace algos::config {
template<typename T>
class Option : public IOption {
public:
    using CondCheckFunc = std::function<bool(T const &val)>;
    using OptCondVector = std::vector<std::pair<CondCheckFunc, std::vector<std::string_view>>>;
    using DefaultFunc = std::function<T()>;
    using NormalizeFunc = std::function<void(T &)>;
    using InstanceCheckFunc = std::function<void(T const &)>;

    Option(OptionInfo const info, T *value_ptr, NormalizeFunc normalize,
           std::optional<T> default_value)
            : info_(info),
              value_ptr_(value_ptr),
              normalize_(normalize),
              default_func_(default_value.has_value()
                            ? [default_value]() { return default_value.value(); }
                            : DefaultFunc{}) {}

    void Set(std::optional<boost::any> value_holder) override;

    T GetValue(std::optional<boost::any> value_holder) const;

    void Unset() override {
        is_set_ = false;
    }

    [[nodiscard]] std::string_view GetName() const override {
        return info_.GetName();
    }

    [[nodiscard]] std::string_view GetDescription() const {
        return info_.GetDescription();
    }

    [[nodiscard]] std::type_index GetTypeIndex() const override {
        return typeid(T);
    }

    [[nodiscard]] bool IsSet() const override {
        return is_set_;
    }

    Option &SetInstanceCheck(InstanceCheckFunc instance_check) {
        instance_check_ = instance_check;
        return *this;
    }

    Option &SetConditionalOpts(OptAddFunc const &add_opts, OptCondVector opt_cond) {
        assert(add_opts);
        assert(!opt_cond.empty());
        opt_cond_ = std::move(opt_cond);
        opt_add_func_ = add_opts;
        return *this;
    }

    Option &OverrideDefaultValue(std::optional<T> new_default) {
        default_func_ = new_default.has_value()
                        ? [new_default]() { return new_default.value(); }
                        : DefaultFunc{};
        return *this;
    }

    Option &OverrideDefaultFunction(DefaultFunc default_func) {
        default_func_ = default_func;
        return *this;
    }

private:
    bool is_set_ = false;
    OptionInfo const info_;
    T *value_ptr_;
    NormalizeFunc normalize_{};
    DefaultFunc default_func_;
    InstanceCheckFunc instance_check_{};
    OptCondVector opt_cond_{};
    OptAddFunc opt_add_func_{};
};

template <typename T>
void Option<T>::Set(std::optional<boost::any> value_holder) {
    assert(!is_set_);
    T value = GetValue(value_holder);
    if (normalize_) normalize_(value);
    if (instance_check_) instance_check_(value);

    assert(value_ptr_ != nullptr);
    *value_ptr_ = value;
    is_set_ = true;
    if (opt_add_func_) {
        for (auto const &[cond, opts]: opt_cond_) {
            if (!cond || cond(value)) {
                opt_add_func_(this, opts);
                break;
            }
        }
    }
}

template <typename T>
T Option<T>::GetValue(std::optional<boost::any> value_holder) const {
    std::string const no_value_no_default =
            std::string("No value was provided to an option without a default value (")
            + info_.GetName().data() + ")";
    if (!value_holder.has_value()) {
        if (!default_func_) throw std::logic_error(no_value_no_default);
        return default_func_();
    } else {
        return boost::any_cast<T>(value_holder.value());
    }
}

}  // namespace algos::config
