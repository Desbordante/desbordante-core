#include "algorithms/primitive.h"

#include <cassert>

namespace algos {

bool Primitive::HandleUnknownOption([[maybe_unused]] std::string_view const& option_name,
                                    [[maybe_unused]] std::optional<boost::any> const& value) {
    return false;
}

bool Primitive::FitCompleted() const {
    return fit_completed_;
}

void Primitive::AddSpecificNeededOptions(
        [[maybe_unused]] std::unordered_set<std::string_view> &previous_options) const {}

void Primitive::MakeExecuteOptsAvailable() {}

config::OptAddFunc Primitive::GetOptAvailFunc() {
    return [this](auto parent_opt, auto children) { MakeOptionsAvailable(parent_opt, children); };
}

void Primitive::ClearOptions() noexcept {
    available_options_.clear();
    opt_parents_.clear();
}

void Primitive::ExecutePrepare() {
    fit_completed_ = true;
    ClearOptions();
    MakeExecuteOptsAvailable();
}

Primitive::Primitive(std::vector<std::string_view> phase_names)
        : phase_names_(std::move(phase_names)) {}

void Primitive::MakeOptionsAvailable(config::IOption *parent,
                                     std::vector<std::string_view> const &option_names) {
    MakeOptionsAvailable(option_names);
    auto it = possible_options_.find(parent->GetName());
    assert(it != possible_options_.end());
    for (const auto &option_name: option_names) {
        opt_parents_[it->first].emplace_back(option_name);
    }
}

void Primitive::ExcludeOptions(std::string_view parent_option) noexcept {
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

void Primitive::UnsetOption(std::string_view option_name) noexcept {
    auto it = possible_options_.find(option_name);
    if (it == possible_options_.end()
        || available_options_.find(it->first) == available_options_.end())
        return;
    it->second->Unset();
    ExcludeOptions(it->first);
}

void Primitive::AddProgress(double const val) noexcept {
    assert(val >= 0);
    std::scoped_lock lock(progress_mutex_);
    cur_phase_progress_ += val;
    assert(cur_phase_progress_ < 101);
}

void Primitive::SetProgress(double const val) noexcept {
    assert(0 <= val && val < 101);
    std::scoped_lock lock(progress_mutex_);
    cur_phase_progress_ = val;
}

void Primitive::MakeOptionsAvailable(const std::vector<std::string_view> &option_names) {
    for (std::string_view name: option_names) {
        auto it = possible_options_.find(name);
        assert(it != possible_options_.end());
        available_options_.insert(it->first);
    }
}

void Primitive::Fit(model::IDatasetStream& data) {
    if (!GetNeededOptions().empty()) throw std::logic_error(
                "All options need to be set before starting processing.");
    FitInternal(data);
    data.Reset();
    ExecutePrepare();
}

unsigned long long Primitive::Execute() {
    if (!fit_completed_) {
        throw std::logic_error("Data must be processed before execution.");
    }
    if (!GetNeededOptions().empty())
        throw std::logic_error("All options need to be set before execution.");
    ResetProgress();
    auto time_ms = ExecuteInternal();
    for (auto const& opt_name : available_options_) {
        possible_options_.at(opt_name)->Unset();
    }
    ClearOptions();
    MakeExecuteOptsAvailable();
    return time_ms;
}

void Primitive::SetOption(std::string_view const& option_name,
                          std::optional<boost::any> const& value) {
    auto it = possible_options_.find(option_name);
    if (it == possible_options_.end()) {
        if (!HandleUnknownOption(option_name, value)) {
            throw std::invalid_argument("Unknown option \"" + std::string{option_name} + '"');
        }
        return;
    } else if (available_options_.find(it->first) == available_options_.end()) {
        throw std::invalid_argument("Invalid option \"" + std::string{option_name} + '"');
    }

    if (it->second->IsSet()) {
        UnsetOption(it->first);
    }

    it->second->Set(value);
}

std::unordered_set<std::string_view> Primitive::GetNeededOptions() const {
    std::unordered_set<std::string_view> needed{};
    for (std::string_view name : available_options_) {
        if (!possible_options_.at(name)->IsSet()) {
            needed.insert(name);
        }
    }
    AddSpecificNeededOptions(needed);
    return needed;
}

std::pair<uint8_t, double> Primitive::GetProgress() const noexcept {
    std::scoped_lock lock(progress_mutex_);
    return std::make_pair(cur_phase_id_, cur_phase_progress_);
}

void Primitive::ToNextProgressPhase() noexcept {
    /* Current phase done, ensure that this is displayed in the progress bar */
    SetProgress(kTotalProgressPercent);

    std::scoped_lock lock(progress_mutex_);
    ++cur_phase_id_;
    assert(cur_phase_id_ < phase_names_.size());
    cur_phase_progress_ = 0;
}

void Primitive::ResetProgress() noexcept {
    cur_phase_id_ = 0;
    cur_phase_progress_ = 0;
}

}  // namespace algos

