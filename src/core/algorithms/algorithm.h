#pragma once

#include <filesystem>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/any.hpp>

#include "config/ioption.h"
#include "config/option.h"
#include "model/table/idataset_stream.h"
#include "parser/csv_parser/csv_parser.h"
#include "util/progress.h"

namespace algos {

class Algorithm {
private:
    util::Progress progress_;
    // All options the algorithm may use
    std::unordered_map<std::string_view, std::unique_ptr<config::IOption>> possible_options_;
    // All options that can be set at the moment
    std::unordered_set<std::string_view> available_options_;
    // Maps a parameter that added other parameters to their names.
    std::unordered_map<std::string_view, std::vector<std::string_view>> opt_parents_;

    bool data_loaded_ = false;

    // Clear the necessary fields for Execute to run repeatedly with different
    // configuration parameters on the same dataset.
    virtual void ResetState() = 0;

    void ExcludeOptions(std::string_view parent_option) noexcept;
    void ClearOptions() noexcept;
    virtual void LoadDataInternal() = 0;
    virtual unsigned long long ExecuteInternal() = 0;

protected:
    void AddProgress(double val) noexcept {
        progress_.AddProgress(val);
    }
    void SetProgress(double val) noexcept {
        progress_.SetProgress(val);
    }
    void ToNextProgressPhase() noexcept {
        progress_.ToNextProgressPhase();
    }

    void MakeOptionsAvailable(std::vector<std::string_view> const& option_names);

    template <typename T>
    void RegisterOption(config::Option<T> option) {
        auto name = option.GetName();
        assert(possible_options_.find(name) == possible_options_.end());
        possible_options_[name] = std::make_unique<config::Option<T>>(std::move(option));
    }

    // Overload this if you want to work with options outside of
    // possible_options_ map. Useful for pipelines.
    virtual bool SetExternalOption(std::string_view option_name, boost::any const& value);
    virtual void AddSpecificNeededOptions(
            std::unordered_set<std::string_view>& previous_options) const;
    void ExecutePrepare();

    // Overload this to add options after your algorithm has processed the data
    // given through LoadData
    virtual void MakeExecuteOptsAvailable();

public:
    constexpr static double kTotalProgressPercent = util::Progress::kTotalProgressPercent;

    Algorithm(Algorithm const& other) = delete;
    Algorithm& operator=(Algorithm const& other) = delete;
    Algorithm(Algorithm&& other) = delete;
    Algorithm& operator=(Algorithm&& other) = delete;
    virtual ~Algorithm() = default;

    // The constructor accepts vector of names of the mining algorithm phases.
    // NOTE: Pass an empty vector here if your algorithm does not have an implemented progress bar.
    explicit Algorithm(std::vector<std::string_view> phase_names);

    void LoadData();

    unsigned long long Execute();

    void SetOption(std::string_view option_name, boost::any const& value = {});

    [[nodiscard]] std::unordered_set<std::string_view> GetNeededOptions() const;

    void UnsetOption(std::string_view option_name) noexcept;

    // See util::Progress::GetProgress description
    std::pair<uint8_t, double> GetProgress() const noexcept {
        return progress_.GetProgress();
    }
    std::vector<std::string_view> const& GetPhaseNames() const noexcept {
        return progress_.GetPhaseNames();
    }

    std::type_index GetTypeIndex(std::string_view option_name) const;
};

}  // namespace algos
