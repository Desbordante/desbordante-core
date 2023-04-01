#include "algorithms/options/configuration.h"

#include <typeinfo>

namespace algos::config {

void Configuration::AddInitialOption(ConfigurationStage stage, std::string_view name) {
    initial_stage_options_[stage].insert(name);
    if (stage == current_stage_) {
        required_options_.insert(name);
    }
}

void Configuration::RegisterOption(std::unique_ptr<config::IOption> option_ptr) {
    std::string_view name = option_ptr->GetName();
    assert(possible_options_.find(name) == possible_options_.end());
    possible_options_[name] = std::move(option_ptr);
}

void Configuration::RegisterOption(std::unique_ptr<config::IOption> option_ptr,
                                   ConfigurationStage stage) {
    std::string_view name = option_ptr->GetName();
    RegisterOption(std::move(option_ptr));
    AddInitialOption(stage, name);
}

ConfigurationStage Configuration::GetCurrentStage() const {
    return current_stage_;
}

void Configuration::StartStage(ConfigurationStage stage) {
    OptionNameSet initial_stage_options = initial_stage_options_[stage];

    for (std::string_view opt_name : initial_stage_options) {
        UnsetOptionInternal(opt_name);
    }

    required_options_ = std::move(initial_stage_options);
    opt_parents_.clear();
    current_stage_ = stage;
}

void Configuration::SetOption(std::string_view option_name, boost::any const& value) {
    if (required_options_.find(option_name) == required_options_.end()) {
        if (set_external_option_) {
            auto [ext_succeeded, ext_exception] = set_external_option_(option_name, value);
            if (ext_succeeded) return;
            if (ext_exception) std::rethrow_exception(ext_exception);
        }
        throw std::invalid_argument("Unknown option \"" + std::string{option_name} + '"');
    }

    auto it = possible_options_.find(option_name);
    assert(it != possible_options_.end());
    std::string_view name = it->first;
    config::IOption& option = *it->second;

    if (set_external_option_) set_external_option_(name, value);

    if (option.IsSet()) {
        UnsetOptionInternal(name);
    }

    std::vector<std::string_view> new_opts = option.Set(value);
    if (new_opts.empty()) return;

    required_options_.insert(new_opts.begin(), new_opts.end());
    std::vector<std::string_view>& child_opt_vec = opt_parents_[name];
    child_opt_vec.insert(child_opt_vec.end(), new_opts.begin(), new_opts.end());
}

std::unordered_set<std::string_view> Configuration::GetNeededOptions() const {
    std::unordered_set<std::string_view> needed{};
    for (std::string_view name : required_options_) {
        if (!possible_options_.at(name)->IsSet()) {
            needed.insert(name);
        }
    }
    if (add_external_needed_options_) add_external_needed_options_(needed);
    return needed;
}

bool Configuration::NeedsOption(std::string_view option_name) const {
    auto it = required_options_.find(option_name);
    if (it == required_options_.end()) return false;

    assert(possible_options_.find(*it) != possible_options_.end());
    return !possible_options_.at(*it)->IsSet();
}

void Configuration::UnsetOption(std::string_view option_name) noexcept {
    if (unset_external_option_) unset_external_option_(option_name);
    UnsetOptionInternal(option_name);
}

void Configuration::UnsetOptionInternal(std::string_view option_name) noexcept {
    auto it = possible_options_.find(option_name);
    if (it == possible_options_.end()) return;

    std::string_view name = it->first;
    IOption& option = *it->second;
    if (required_options_.find(name) == required_options_.end()) return;
    option.Unset();
    UnsetChildren(name);
}

void Configuration::UnsetChildren(std::string_view parent_option_name) noexcept {
    auto parent_it = opt_parents_.find(parent_option_name);
    if (parent_it == opt_parents_.end()) return;

    for (std::string_view option_name : parent_it->second) {
        auto possible_opt_it = possible_options_.find(option_name);
        assert(possible_opt_it != possible_options_.end());
        std::string_view name = possible_opt_it->first;
        required_options_.erase(name);
        UnsetOptionInternal(name);
    }
    opt_parents_.erase(parent_it);
}

std::type_index Configuration::GetTypeIndex(std::string_view option_name) const {
    static const std::type_index void_index = typeid(void);

    if (NeedsOption(option_name)) return possible_options_.at(option_name)->GetTypeIndex();
    if (!get_external_type_index_) {
        auto it = possible_options_.find(option_name);
        return it == possible_options_.end() ? void_index : it->second->GetTypeIndex();
    }
    return get_external_type_index_(option_name);
}

bool Configuration::IsInitialAtStage(std::string_view option_name,
                                     algos::config::ConfigurationStage stage) const {
    OptionNameSet const& stage_opts = initial_stage_options_[stage];
    return stage_opts.find(option_name) != stage_opts.end();
}

void Configuration::SetExternalOptionFunctions(
        AddExternalOptFunc add_external_needed_options, SetExternalOptFunc set_external_option,
        UnsetExternalOptFunc unset_external_option,
        GetExternalTypeIndexFunc get_external_type_index) noexcept {
    assert(!(add_external_needed_options_ || set_external_option_ || unset_external_option_ ||
             get_external_type_index_));
    assert(add_external_needed_options && set_external_option && unset_external_option &&
           get_external_type_index);
    add_external_needed_options_ = std::move(add_external_needed_options);
    set_external_option_ = std::move(set_external_option);
    unset_external_option_ = std::move(unset_external_option);
    get_external_type_index_ = std::move(get_external_type_index);
}

}  // namespace algos::config
