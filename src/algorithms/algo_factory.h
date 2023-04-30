#pragma once

#include <type_traits>

#include <boost/any.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/program_options.hpp>
#include <enum.h>

#include "algorithms/algorithms.h"
#include "algorithms/create_algorithm.h"
#include "algorithms/legacy_algorithms.h"
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

template <typename ParamsMap>
ACAlgorithm::Config CreateAcAlgorithmConfigFromMap(ParamsMap params) {
    namespace onam = util::config::names;
    ACAlgorithm::Config c;

    c.data = ExtractOptionValue<std::filesystem::path>(params, onam::kData);
    c.separator = ExtractOptionValue<char>(params, onam::kSeparator);
    c.has_header = ExtractOptionValue<bool>(params, onam::kHasHeader);
    c.bin_operation = ExtractOptionValue<char>(params, onam::kBinaryOperation);
    c.fuzziness = ExtractOptionValue<double>(params, onam::kFuzziness);
    if (c.fuzziness <= 0 || c.fuzziness > 1) {
        throw std::invalid_argument(
                "Fuzziness value must belong to the interval: (0, 1]");
    }
    c.p_fuzz = ExtractOptionValue<double>(params, onam::kFuzzinessProbability);
    if (c.p_fuzz <= 0 || c.p_fuzz > 1) {
        throw std::invalid_argument(
                "FuzzinessProbability value must belong to the interval: (0, 1]");
    }
    c.weight = ExtractOptionValue<double>(params, onam::kWeight);
    if (c.weight <= 0 || c.weight > 1) {
        throw std::invalid_argument("Weight value must belong to the interval: (0, 1]");
    }
    c.bumps_limit = ExtractOptionValue<size_t>(params, onam::kBumpsLimit);
    c.iterations_limit = ExtractOptionValue<size_t>(params, onam::kIterationsLimit);
    if (c.iterations_limit < 1) {
        throw std::invalid_argument("IterationsLimit value should not be less than one");
    }
    c.pairing_rule = ExtractOptionValue<std::string>(params, onam::kPairingRule);
    c.seed = ExtractOptionValue<int>(params, onam::kACSeed);

    return c;
}

template <typename ParamsMap>
std::unique_ptr<Algorithm> CreateAcAlgorithmInstance(ParamsMap&& params) {
    ACAlgorithm::Config const config =
        CreateAcAlgorithmConfigFromMap(std::forward<ParamsMap>(params));
    return std::make_unique<ACAlgorithm>(config);
}

}  // namespace details

void ConfigureFromFunction(Algorithm& algorithm,
                           std::function<boost::any(std::string_view)> const& get_value);

template <typename OptionMap>
boost::any GetOrEmpty(OptionMap const& options, std::string_view option_name) {
    auto it = options.find(std::string{option_name});
    return it == options.end() ? boost::any{} : it->second;
}

template <typename OptionMap>
void ConfigureFromMap(Algorithm& algorithm, OptionMap&& options) {
    ConfigureFromFunction(algorithm, [&options](std::string_view option_name) -> boost::any {
        return GetOrEmpty(options, option_name);
    });
}

template <typename OptionMap>
void LoadAlgorithm(Algorithm& algorithm, OptionMap&& options) {
    ConfigureFromFunction(algorithm, [&options](std::string_view option_name) {
        using namespace util::config::names;
        if (option_name == kData) {
            RelationStream parser = std::make_shared<CSVParser>(
                    details::ExtractOptionValue<std::filesystem::path>(options, kData),
                    details::ExtractOptionValue<char>(options, kSeparator),
                    details::ExtractOptionValue<bool>(options, kHasHeader));
            return boost::any{parser};
        }
        return GetOrEmpty(options, option_name);
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
    AlgorithmType precise_algo = details::ExtractOptionValue<AlgorithmType>(
            options, util::config::names::kPreciseAlgorithm);
    AlgorithmType approx_algo = details::ExtractOptionValue<AlgorithmType>(
            options, util::config::names::kApproximateAlgorithm);
    std::unique_ptr<TypoMiner> typo_miner = std::make_unique<TypoMiner>(precise_algo, approx_algo);
    LoadAlgorithm(*typo_miner, std::forward<OptionMap>(options));
    return typo_miner;
}

template <typename OptionMap>
std::unique_ptr<Algorithm> CreateAlgorithm(std::string const& algorithm_name, OptionMap&& options) {
    if (algorithm_name == "ac") {
        return details::CreateAcAlgorithmInstance(std::forward<OptionMap>(options));
    }
    if (algorithm_name == "typo_miner") {
        return CreateTypoMiner(std::forward<OptionMap>(options));
    }
    AlgorithmType const algorithm_enum = AlgorithmType::_from_string_nocase(algorithm_name.c_str());
    return CreateAlgorithm(algorithm_enum, std::forward<OptionMap>(options));
}

}  // namespace algos
