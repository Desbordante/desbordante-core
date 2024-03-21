#include "ind_algorithm.h"

#include "config/names_and_descriptions.h"
#include "config/tabular_data/input_tables/option.h"

namespace algos {

INDAlgorithm::INDAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    RegisterOption(config::kTablesOpt(&input_tables_));
    MakeOptionsAvailable({config::kTablesOpt.GetName()});
}

}  // namespace algos
