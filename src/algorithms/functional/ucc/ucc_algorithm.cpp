#include "algorithms/functional/ucc/ucc_algorithm.h"

#include "config/equal_nulls/option.h"
#include "config/tabular_data/input_table/option.h"

namespace algos {

UCCAlgorithm::UCCAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    RegisterOptions();
    MakeOptionsAvailable({config::TableOpt.GetName(), config::EqualNullsOpt.GetName()});
}

void UCCAlgorithm::RegisterOptions() {
    RegisterOption(config::TableOpt(&input_table_));
    RegisterOption(config::EqualNullsOpt(&is_null_equal_null_));
}

}  // namespace algos
