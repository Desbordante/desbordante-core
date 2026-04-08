#include "core/algorithms/ucc/ucc_algorithm.h"

#include <memory>
#include <vector>

#include "core/config/tabular_data/input_table/option.h"
#include "core/config/common_option.h"

namespace algos {

UCCAlgorithm::UCCAlgorithm() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void UCCAlgorithm::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
}

}  // namespace algos
