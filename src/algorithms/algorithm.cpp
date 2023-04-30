#include "algorithms/algorithm.h"

#include <cassert>

namespace algos {

bool Algorithm::HandleUnknownOption([[maybe_unused]] std::string_view option_name,
                                    [[maybe_unused]] boost::any const& value) {
    return false;
}

bool Algorithm::DataLoaded() const {
    return data_loaded_;
}

void Algorithm::AddSpecificNeededOptions(
        [[maybe_unused]] std::unordered_set<std::string_view>& previous_options) const {}

void Algorithm::MakeExecuteOptsAvailable() {}

void Algorithm::ClearOptions() noexcept {
    available_options_.clear();
    opt_parents_.clear();
}

void Algorithm::ExecutePrepare() {
    data_loaded_ = true;
    ClearOptions();
    MakeExecuteOptsAvailable();
}

Algorithm::Algorithm(std::vector<std::string_view> phase_names)
    : progress_(std::move(phase_names)) {}

void Algorithm::ExcludeOptions(std::string_view parent_option) noexcept {
    auto it = opt_parents_.find(parent_option);
    if (it == opt_parents_.end()) return;

    for (auto const &option_name: it->second) {
        auto possible_opt_it = possible_options_.find(option_name);
        assert(possible_opt_it != possible_options_.end());
        available_options_.erase(possible_opt_it->first);
        UnsetOption(possible_opt_it->first);
    }
    opt_parents_.erase(it);
}

void Algorithm::UnsetOption(std::string_view option_name) noexcept {
    auto it = possible_options_.find(option_name);
    if (it == possible_options_.end()
        || available_options_.find(it->first) == available_options_.end())
        return;
    it->second->Unset();
    ExcludeOptions(it->first);
}

void Algorithm::MakeOptionsAvailable(std::vector<std::string_view> const& option_names) {
    for (std::string_view name: option_names) {
        auto it = possible_options_.find(name);
        assert(it != possible_options_.end());
        available_options_.insert(it->first);
    }
}

void Algorithm::LoadData() {
    if (!GetNeededOptions().empty()) throw std::logic_error(
                "All options need to be set before starting processing.");
    LoadDataInternal();
    ExecutePrepare();
}

unsigned long long Algorithm::Execute() {
    if (!data_loaded_) {
        throw std::logic_error("Data must be processed before execution.");
    }
    if (!GetNeededOptions().empty())
        throw std::logic_error("All options need to be set before execution.");
    progress_.ResetProgress();
    ResetState();
    auto time_ms = ExecuteInternal();
    for (auto const& opt_name : available_options_) {
        possible_options_.at(opt_name)->Unset();
    }
    ClearOptions();
    MakeExecuteOptsAvailable();
    return time_ms;
}

void Algorithm::SetOption(std::string_view option_name, boost::any const& value) {
    auto it = possible_options_.find(option_name);
    if (it == possible_options_.end()) {
        if (!HandleUnknownOption(option_name, value)) {
            throw std::invalid_argument("Unknown option \"" + std::string{option_name} + '"');
        }
        return;
    }
    std::string_view name = it->first;
    util::config::IOption& option = *it->second;
    if (available_options_.find(name) == available_options_.end()) {
        throw std::invalid_argument("Invalid option \"" + std::string{name} + '"');
    }

    if (option.IsSet()) {
        UnsetOption(name);
    }

    std::vector<std::string_view> const& new_opts = option.Set(value);
    if (new_opts.empty()) return;
    MakeOptionsAvailable(new_opts);
    std::vector<std::string_view>& child_opts = opt_parents_[name];
    child_opts.insert(child_opts.end(), new_opts.begin(), new_opts.end());
}

std::unordered_set<std::string_view> Algorithm::GetNeededOptions() const {
    std::unordered_set<std::string_view> needed{};
    for (std::string_view name : available_options_) {
        if (!possible_options_.at(name)->IsSet()) {
            needed.insert(name);
        }
    }
    AddSpecificNeededOptions(needed);
    return needed;
}

std::type_index Algorithm::GetTypeIndex(std::string_view option_name) const {
    auto it = possible_options_.find(option_name);
    if (it == possible_options_.end()) return typeid(void);
    return it->second->GetTypeIndex();
}

}  // namespace algos

