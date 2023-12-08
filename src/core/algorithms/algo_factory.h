#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>

#include <boost/any.hpp>

#include "algorithms/algorithm.h"
#include "algorithms/algorithm_types.h"

namespace algos {

using StdParamsMap = std::unordered_map<std::string, boost::any>;

template <typename FuncType>
void ConfigureFromFunction(Algorithm& algorithm, FuncType get_opt_value_by_name) {
    std::unordered_set<std::string_view> needed;
    while (!(needed = algorithm.GetNeededOptions()).empty()) {
        for (std::string_view option_name : needed) {
            algorithm.SetOption(option_name, get_opt_value_by_name(option_name));
        }
    }
}

void ConfigureFromMap(Algorithm& algorithm, StdParamsMap const& options);
void LoadAlgorithm(Algorithm& algorithm, StdParamsMap const& options);

template <typename T>
std::unique_ptr<T> CreateAndLoadAlgorithm(StdParamsMap const& options) {
    std::unique_ptr<T> algorithm = std::make_unique<T>();
    LoadAlgorithm(*algorithm, options);
    return algorithm;
}

std::unique_ptr<Algorithm> CreateAlgorithm(AlgorithmType algorithm_enum,
                                           StdParamsMap const& options);
std::unique_ptr<Algorithm> CreateTypoMiner(StdParamsMap const& options);
std::unique_ptr<Algorithm> CreateAlgorithm(std::string const& algorithm_name,
                                           StdParamsMap const& options);
}  // namespace algos
