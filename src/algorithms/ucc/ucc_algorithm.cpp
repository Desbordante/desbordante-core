#include "algorithms/ucc/ucc_algorithm.h"

#include "util/config/equal_nulls/option.h"

namespace algos {

void UCCAlgorithm::RegisterOptions() {
    RegisterOption(util::config::EqualNullsOpt(&is_null_equal_null_));
}

}  // namespace algos
