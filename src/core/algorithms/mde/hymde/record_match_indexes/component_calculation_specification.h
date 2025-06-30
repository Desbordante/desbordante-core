#pragma once

#include <memory>
#include <vector>

#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator.h"

namespace algos::hymde::record_match_indexes {
using ComponentCalculationSpecification =
        std::vector<std::shared_ptr<calculators::Calculator::Creator>>;
}  // namespace algos::hymde::record_match_indexes
