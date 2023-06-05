#pragma once

#include <type_traits>

#include <boost/any.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/program_options.hpp>
#include <enum.h>

#include "algorithms/algorithms.h"
#include "algorithms/create_algorithm.h"
#include "algorithms/typo_miner.h"
#include "util/config/names.h"

namespace algos {

using StdParamsMap = std::unordered_map<std::string, boost::any>;

namespace details {

template <typename OptionMap>
boost::any ExtractAnyFromMap(OptionMap& options, std::string_view option_name) {
    using std::is_same_v, std::decay, boost::program_options::variables_map;
    const std::string string_opt{option_name};
    auto it = options.find(string_opt);
    if (it == options.end()) {
        throw std::out_of_range("No option named \"" + string_opt + "\" in parameters.");
    }
    if constexpr (is_same_v<typename decay<OptionMap>::type, variables_map>) {
        return options.extract(it).mapped().value();
    } else {
        return options.extract(it).mapped();
    }
}

template <typename T, typename OptionMap>
T ExtractOptionValue(OptionMap&& options, std::string const& option_name) {
    return boost::any_cast<T>(ExtractAnyFromMap(std::forward<OptionMap>(options), option_name));
}

template <typename OptionMap>
boost::any GetOrEmpty(OptionMap const& options, std::string_view option_name) {
    auto it = options.find(std::string{option_name});
    return it == options.end() ? boost::any{} : it->second;
}

}  // namespace details

template <typename FuncType>
void ConfigureFromFunction(Algorithm& algorithm, FuncType get_opt_value_by_name) {
    std::unordered_set<std::string_view> needed;
    while (!(needed = algorithm.GetNeededOptions()).empty()) {
        for (std::string_view option_name : needed) {
            algorithm.SetOption(option_name, get_opt_value_by_name(option_name));
        }
    }
}

template <typename OptionMap>
void ConfigureFromMap(Algorithm& algorithm, OptionMap&& options) {
    ConfigureFromFunction(algorithm, [&options](std::string_view option_name) {
        return details::GetOrEmpty(options, option_name);
    });
}

template <typename OptionMap>
void LoadAlgorithm(Algorithm& algorithm, OptionMap&& options) {
    ConfigureFromFunction(algorithm, [&options](std::string_view option_name) {
        using namespace util::config::names;
        if (option_name == kTable && options.find(std::string{kTable}) == options.end()) {
            util::config::InputTable parser = std::make_shared<CSVParser>(
                    details::ExtractOptionValue<std::filesystem::path>(options, kData),
                    details::ExtractOptionValue<char>(options, kSeparator),
                    details::ExtractOptionValue<bool>(options, kHasHeader));
            return boost::any{parser};
        }
        return details::GetOrEmpty(options, option_name);
    });
    algorithm.LoadData();
    ConfigureFromMap(algorithm, options);
}

template <typename T, typename OptionMap>
std::unique_ptr<T> CreateAndLoadAlgorithm(OptionMap&& options) {
    std::unique_ptr<T> algorithm = std::make_unique<T>();
    LoadAlgorithm(*algorithm, std::forward<OptionMap>(options));
    return algorithm;
}

template <typename OptionMap>
std::unique_ptr<Algorithm> CreateAlgorithm(AlgorithmType algorithm_enum, OptionMap&& options) {
    std::unique_ptr<Algorithm> algorithm = CreateAlgorithmInstance(algorithm_enum);
    LoadAlgorithm(*algorithm, std::forward<OptionMap>(options));
    return algorithm;
}

template <typename OptionMap>
std::unique_ptr<Algorithm> CreateTypoMiner(OptionMap&& options) {
    using details::ExtractOptionValue;
    using util::config::names::kPreciseAlgorithm, util::config::names::kApproximateAlgorithm;
    AlgorithmType precise_algo = ExtractOptionValue<AlgorithmType>(options, kPreciseAlgorithm);
    AlgorithmType approx_algo = ExtractOptionValue<AlgorithmType>(options, kApproximateAlgorithm);
    std::unique_ptr<TypoMiner> typo_miner = std::make_unique<TypoMiner>(precise_algo, approx_algo);
    LoadAlgorithm(*typo_miner, std::forward<OptionMap>(options));
    return typo_miner;
}

template <typename OptionMap>
std::unique_ptr<Algorithm> CreateAlgorithm(std::string const& algorithm_name, OptionMap&& options) {
    if (algorithm_name == "typo_miner") {
        return CreateTypoMiner(std::forward<OptionMap>(options));
    }
    AlgorithmType const algorithm_enum = AlgorithmType::_from_string_nocase(algorithm_name.c_str());
    return CreateAlgorithm(algorithm_enum, std::forward<OptionMap>(options));
}

}  // namespace algos
