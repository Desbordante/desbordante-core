#pragma once

#include "algorithms/options/descriptions.h"
#include "algorithms/options/names.h"
#include "algorithms/options/type.h"

namespace algos::config {

using ErrorType = double;
const OptionType<ErrorType> ErrorOpt{
        {config::names::kError, config::descriptions::kDError}, 0.01, [](auto value) {
            if (!(value >= 0 && value <= 1)) {
                throw std::invalid_argument("ERROR: error should be between 0 and 1.");
            }
        }};

}  // namespace algos::config
