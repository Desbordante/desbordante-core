#include "core/algorithms/ucc/ucc_algorithm.h"

#include "core/config/equal_nulls/option.h"
#include "core/config/tabular_data/input_table/option.h"

namespace algos {

UCCAlgorithm::UCCAlgorithm() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void UCCAlgorithm::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
}

}  // namespace algos
