#include "algorithms/algo_factory.h"

namespace algos {

void ConfigureFromFunction(Algorithm& algorithm,
                           std::function<boost::any(std::string_view)> const& get_value) {
    std::unordered_set<std::string_view> needed;
    while (!(needed = algorithm.GetNeededOptions()).empty()) {
        for (std::string_view option_name : needed) {
            algorithm.SetOption(option_name, get_value(option_name));
        }
    }
}

}  // namespace algos
