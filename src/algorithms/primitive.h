#pragma once

#include <filesystem>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/any.hpp>

#include "algorithms/options/configuration.h"
#include "algorithms/options/ioption.h"
#include "algorithms/options/option.h"
#include "model/idataset_stream.h"
#include "parser/csv_parser.h"
#include "util/progress.h"

namespace algos {

class Primitive {
private:
    util::Progress progress_;

    bool fit_completed_ = false;

    // Clear the necessary fields for Execute to run repeatedly with different
    // configuration parameters on the same dataset.
    virtual void ResetState() = 0;

    virtual void FitInternal(model::IDatasetStream& data_stream) = 0;
    virtual unsigned long long ExecuteInternal() = 0;

protected:
    config::Configuration configuration_;

    void AddProgress(double val) noexcept {
        progress_.AddProgress(val);
    }
    void SetProgress(double val) noexcept {
        progress_.SetProgress(val);
    }
    void ToNextProgressPhase() noexcept {
        progress_.ToNextProgressPhase();
    }

    template <typename T>
    void RegisterOption(config::Option<T> option) {
        configuration_.RegisterOption(option);
    }

    template <typename T>
    void RegisterInitialFitOption(config::Option<T> option) {
        configuration_.RegisterOption(option, config::ConfigurationStage::fit);
    }

    template <typename T>
    void RegisterInitialExecuteOption(config::Option<T> option) {
        configuration_.RegisterOption(option, config::ConfigurationStage::execute);
    }

public:
    constexpr static double kTotalProgressPercent = util::Progress::kTotalProgressPercent;

    Primitive(Primitive const& other) = delete;
    Primitive& operator=(Primitive const& other) = delete;
    Primitive(Primitive&& other) = delete;
    Primitive& operator=(Primitive&& other) = delete;
    virtual ~Primitive() = default;

    // The constructor accepts vector of names of the mining algorithm phases.
    // NOTE: Pass an empty vector here if your algorithm does not have an implemented progress bar.
    explicit Primitive(std::vector<std::string_view> phase_names);

    void Fit(model::IDatasetStream & data_stream);
    bool FitCompleted() const;

    unsigned long long Execute();

    void SetOption(std::string_view option_name, boost::any const& value = {}) {
        configuration_.SetOption(option_name, value);
    }

    [[nodiscard]] std::unordered_set<std::string_view> GetNeededOptions() const {
        return configuration_.GetNeededOptions();
    }

    void UnsetOption(std::string_view option_name) noexcept {
        configuration_.UnsetOption(option_name);
    }

    [[nodiscard]] std::type_index GetTypeIndex(std::string_view option_name) const {
        return configuration_.GetTypeIndex(option_name);
    }

    [[nodiscard]] bool NeedsOption(std::string_view option_name) const {
        return configuration_.NeedsOption(option_name);
    }

    [[nodiscard]] bool IsInitialAtStage(std::string_view option_name,
                                        config::ConfigurationStage stage) const {
        return configuration_.IsInitialAtStage(option_name, stage);
    }

    // See util::Progress::GetProgress description
    std::pair<uint8_t, double> GetProgress() const noexcept {
        return progress_.GetProgress();
    }
    std::vector<std::string_view> const& GetPhaseNames() const noexcept {
        return progress_.GetPhaseNames();
    }
};

}  // namespace algos
