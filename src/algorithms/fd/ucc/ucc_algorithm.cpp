#include "algorithms/fd/ucc/ucc_algorithm.h"

#include "util/config/equal_nulls/option.h"
#include "util/config/tabular_data/input_table/option.h"

namespace algos {

UCCAlgorithm::UCCAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    RegisterOptions();
    MakeOptionsAvailable({util::config::TableOpt.GetName(), util::config::EqualNullsOpt.GetName()});
}

void UCCAlgorithm::RegisterOptions() {
    RegisterOption(util::config::TableOpt(&input_table_));
    RegisterOption(util::config::EqualNullsOpt(&is_null_equal_null_));
}

}  // namespace algos
