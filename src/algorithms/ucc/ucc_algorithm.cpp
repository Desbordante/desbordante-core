#include "algorithms/ucc/ucc_algorithm.h"

#include "algorithms/options/equal_nulls/option.h"

namespace algos {

void UCCAlgorithm::RegisterOptions() {
    RegisterInitialFitOption(config::EqualNullsOpt(&is_null_equal_null_));
}

}  // namespace algos
