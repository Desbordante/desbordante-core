#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>

#include <boost/any.hpp>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/algorithm_types.h"

namespace algos {

using StdParamsMap = std::unordered_map<std::string, boost::any>;

template <typename FuncType>
void ConfigureFromFunction(Algorithm& algorithm, FuncType get_opt_value_by_name) {
    std::unordered_set<std::string_view> needed;
    while (!(needed = algorithm.GetNeededOptions()).empty()) {
        std::vector<std::string_view> needed_but_empty;
        for (std::string_view option_name : needed) {
            boost::any value = get_opt_value_by_name(option_name);
            if (value.empty()) {
                needed_but_empty.push_back(option_name);
                continue;
            }
            algorithm.SetOption(option_name, value);
        }

        // After we set some other options these options may become non-required
        for (std::string_view option_name : needed_but_empty) {
            if (algorithm.OptionIsRequired(option_name)) {
                algorithm.SetOption(option_name, boost::any{});
            }
        }
    }
}

void ConfigureFromMap(Algorithm& algorithm, StdParamsMap const& options);
void LoadAlgorithmData(Algorithm& algorithm, StdParamsMap const& options);
void LoadAlgorithm(Algorithm& algorithm, StdParamsMap const& options);

template <typename T>
std::unique_ptr<T> CreateAndLoadAlgorithm(StdParamsMap const& options) {
    std::unique_ptr<T> algorithm = std::make_unique<T>();
    LoadAlgorithm(*algorithm, options);
    return algorithm;
}
}  // namespace algos
