#include "algo_factory.h"

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "algorithms/algorithms.h"
#include "algorithms/create_algorithm.h"
#include "config/names.h"
#include "tabular_data/input_tables_type.h"

namespace algos {

namespace {

boost::any GetAnyFromMap(StdParamsMap const& options, std::string_view option_name) {
    using std::is_same_v, std::decay;
    std::string const string_opt{option_name};
    auto it = options.find(string_opt);
    if (it == options.end()) {
        throw std::out_of_range("No option named \"" + string_opt + "\" in parameters.");
    }
    return it->second;
}

template <typename T>
T GetOptionValue(StdParamsMap const& options, std::string const& option_name) {
    return boost::any_cast<T>(GetAnyFromMap(options, option_name));
}

boost::any GetOrEmpty(StdParamsMap const& options, std::string_view option_name) {
    auto it = options.find(std::string{option_name});
    return it == options.end() ? boost::any{} : it->second;
}

}  // namespace

void ConfigureFromMap(Algorithm& algorithm, StdParamsMap const& options) {
    ConfigureFromFunction(algorithm, [&options](std::string_view option_name) {
        return GetOrEmpty(options, option_name);
    });
}

void LoadAlgorithmData(Algorithm& algorithm, StdParamsMap const& options) {
    ConfigureFromFunction(algorithm, [&options](std::string_view option_name) {
        using namespace config::names;
        auto create_input_table = [](CSVConfig const& csv_config) -> config::InputTable {
            return std::make_shared<CSVParser>(csv_config);
        };

        if (option_name == kTable && options.find(std::string{kTable}) == options.end()) {
            auto table = create_input_table(GetOptionValue<CSVConfig>(options, kCsvConfig));
            return boost::any{table};
        } else if (option_name == kTables && options.find(std::string{kTables}) == options.end()) {
            auto csv_configs = GetOptionValue<std::vector<CSVConfig>>(options, kCsvConfigs);
            config::InputTables tables;
            tables.reserve(csv_configs.size());
            std::transform(csv_configs.begin(), csv_configs.end(), std::back_inserter(tables),
                           create_input_table);

            return boost::any{tables};
        }
        return GetOrEmpty(options, option_name);
    });
    algorithm.LoadData();
}

void LoadAlgorithm(Algorithm& algorithm, StdParamsMap const& options) {
    LoadAlgorithmData(algorithm, options);
    ConfigureFromMap(algorithm, options);
}
}  // namespace algos
