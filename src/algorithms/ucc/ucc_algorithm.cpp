#include "algorithms/ucc/ucc_algorithm.h"

namespace algos {

void UCCAlgorithm::RegisterOptions() {
    RegisterOption(config::EqualNullsOpt(&is_null_equal_null_));
}

}  // namespace algos
