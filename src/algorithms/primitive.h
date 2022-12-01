#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/any.hpp>

#include "algorithms/options/ioption.h"
#include "algorithms/options/opt_add_func_type.h"
#include "algorithms/options/option.h"
#include "model/idataset_stream.h"
#include "parser/csv_parser.h"

namespace algos {

class Primitive {
private:
    std::mutex mutable progress_mutex_;
    double cur_phase_progress_ = 0;
    uint8_t cur_phase_id_ = 0;
    std::unordered_map<std::string_view, std::unique_ptr<config::IOption>> possible_options_{};
    std::unordered_set<std::string_view> available_options;
    std::unordered_map<std::string_view, std::vector<std::string_view>> opt_parents_{};
    bool fit_completed_ = false;

    void MakeOptionsAvailable(config::IOption *parent_name,
                              std::vector<std::string_view> const& option_names);

    void ExcludeOptions(std::string_view parent_option) noexcept;
    void ClearOptions() noexcept;
    virtual void FitInternal(model::IDatasetStream& data_stream) = 0;
    virtual unsigned long long ExecuteInternal() = 0;

protected:
    /* Vector of names of algorithm phases, should be initialized in a constructor
     * if algorithm has more than one phase. This vector is used to determine the
     * total number of phases.
     * Use empty vector to intialize this field if your algorithm does not have
     * implemented progress bar.
     */
    std::vector<std::string_view> const phase_names_;

    void AddProgress(double const val) noexcept;
    void SetProgress(double const val) noexcept;
    void ToNextProgressPhase() noexcept;
    void MakeOptionsAvailable(std::vector<std::string_view> const& option_names);

    template<typename T>
    void RegisterOption(config::Option<T> option) {
        auto name = option.GetName();
        assert(possible_options_.find(name) == possible_options_.end());
        possible_options_[name] = std::make_unique<config::Option<T>>(std::move(option));
    }

    config::OptAddFunc GetOptAvailFunc();

    // Overload this if you want to work with options outside of
    // possible_options_ map. Useful for pipelines.
    virtual bool HandleUnknownOption(std::string_view const& option_name,
                                     std::optional<boost::any> const& value);
    virtual void AddSpecificNeededOptions(
            std::unordered_set<std::string_view>& previous_options) const;
    void ExecutePrepare();

    // Overload this to add options after your primitive has processed the data
    // given through Fit
    virtual void MakeExecuteOptsAvailable();

public:
    constexpr static double kTotalProgressPercent = 100.0;

    Primitive(Primitive const& other) = delete;
    Primitive& operator=(Primitive const& other) = delete;
    Primitive(Primitive&& other) = delete;
    Primitive& operator=(Primitive&& other) = delete;
    virtual ~Primitive() = default;

    explicit Primitive(std::vector<std::string_view> phase_names);

    void Fit(model::IDatasetStream & data_stream);
    bool FitCompleted() const;

    unsigned long long Execute();

    void SetOption(std::string_view const& option_name,
                   std::optional<boost::any> const& value = {});

    [[nodiscard]] std::unordered_set<std::string_view> GetNeededOptions() const;

    void UnsetOption(std::string_view option_name) noexcept;

    /* Returns pair with current progress state.
     * Pair has the form <current phase id, current phase progess>
     */
    std::pair<uint8_t, double> GetProgress() const noexcept;

    std::vector<std::string_view> const& GetPhaseNames() const noexcept {
        return phase_names_;
    }

    std::type_index GetTypeIndex(std::string_view option_name) const {
        return possible_options_.at(option_name)->GetTypeIndex();
    }
};

}  // namespace algos
