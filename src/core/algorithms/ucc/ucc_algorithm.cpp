#include "ucc_algorithm.h"

#include <memory>   // for shared_ptr
#include <utility>  // for move

#include <boost/type_index/type_index_facade.hpp>  // for operator==

#include "algorithm.h"                               // for Algorithm
#include "common_option.h"                           // for CommonOption
#include "config/equal_nulls/option.h"               // for kEqualNullsOpt
#include "config/tabular_data/input_table/option.h"  // for kTableOpt

namespace algos {

UCCAlgorithm::UCCAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void UCCAlgorithm::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
}

}  // namespace algos
