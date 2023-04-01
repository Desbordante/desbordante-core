#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/any.hpp>

#include "algorithms/options/configuration_stage.h"
#include "algorithms/options/ioption.h"
#include "algorithms/options/option.h"

namespace algos::config {

class Configuration {
public:
    using OptionNameSet = std::unordered_set<std::string_view>;
    using AddExternalOptFunc = std::function<void(OptionNameSet&)>;
    using SetExternalOptFunc =
            std::function<std::pair<bool, std::exception_ptr>(std::string_view, boost::any const&)>;
    using UnsetExternalOptFunc = std::function<void(std::string_view)>;
    using GetExternalTypeIndexFunc = std::function<std::type_index(std::string_view)>;

private:
    // All options the algorithm may use.
    std::unordered_map<std::string_view, std::unique_ptr<config::IOption>> possible_options_{};
    // All options that should be set for the algorithm to proceed (may change
    // as some options are set).
    OptionNameSet required_options_;
    // Maps an option's name to the names of options it made required.
    std::unordered_map<std::string_view, std::vector<std::string_view>> opt_parents_{};
    ConfigurationStage current_stage_ = ConfigurationStage::fit;
    // Options that will become available after the corresponding stage is started.
    std::array<OptionNameSet, ConfigurationStage::_size()> initial_stage_options_;

    // Pipeline-related functions.
    // Adds names of options that are needed for algorithms inside a pipeline.
    AddExternalOptFunc add_external_needed_options_;
    // Tries to set value of an option of an algorithm inside a pipeline.
    SetExternalOptFunc set_external_option_;
    // Tries to unset value of an option of an algorithm inside a pipeline.
    UnsetExternalOptFunc unset_external_option_;
    // Gets the type index corresponding to the type of the option of an
    // algorithm inside a pipeline.
    GetExternalTypeIndexFunc get_external_type_index_;

    void UnsetChildren(std::string_view parent_option_name) noexcept;

    void UnsetOptionInternal(std::string_view option_name) noexcept;

    // Add an option that becomes available as soon as a configuration stage
    // starts. This option is called "initial".
    void AddInitialOption(ConfigurationStage stage, std::string_view name);

public:
    void RegisterOption(std::unique_ptr<config::IOption> option_ptr);

    void RegisterOption(std::unique_ptr<config::IOption> option_ptr, ConfigurationStage stage);

    template <typename T>
    void RegisterOption(config::Option<T> option) {
        RegisterOption(std::make_unique<config::Option<T>>(std::move(option)));
    }

    template <typename T>
    void RegisterOption(config::Option<T> option, ConfigurationStage stage) {
        RegisterOption(std::make_unique<config::Option<T>>(std::move(option)), stage);
    }

    // The pipeline should take care that its algorithms are called in the
    // correct way, so this method doesn't take its algorithms into account.
    void StartStage(ConfigurationStage stage);

    [[nodiscard]] ConfigurationStage GetCurrentStage() const;

    void SetOption(std::string_view option_name, boost::any const& value = {});

    void UnsetOption(std::string_view option_name) noexcept;

    // Get all options that are required and not set.
    [[nodiscard]] OptionNameSet GetNeededOptions() const;

    // Checks if the provided option has to be set to proceed. Does not account
    // for options from the algorithms in a pipeline.
    [[nodiscard]] bool NeedsOption(std::string_view option_name) const;

    // Check if an option is initial (see above) at the provided configuration
    // stage. Does not account for the algorithms in a pipeline.
    [[nodiscard]] bool IsInitialAtStage(std::string_view option_name,
                                        ConfigurationStage stage) const;

    // In a normal algorithm: gets the type index of the option with the given name.
    // In a pipeline: gets the type index of the pipeline's option with the
    // given name if it is required, otherwise gets the type index its
    // algorithms need.
    [[nodiscard]] std::type_index GetTypeIndex(std::string_view option_name) const;

    // Add custom handling to some configuration aspects. Intended for use by a
    // pipeline.
    void SetExternalOptionFunctions(AddExternalOptFunc add_external_needed_options,
                                    SetExternalOptFunc set_external_option,
                                    UnsetExternalOptFunc unset_external_option,
                                    GetExternalTypeIndexFunc get_external_type_index) noexcept;
};

}  // namespace algos::config
