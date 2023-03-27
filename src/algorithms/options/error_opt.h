#pragma once

#include "algorithms/options/common_option.h"
#include "algorithms/options/names_and_descriptions.h"

namespace algos::config {

using ErrorType = double;
const CommonOption<ErrorType> ErrorOpt{
        {config::names::kError, config::descriptions::kDError}, 0.01, [](auto value) {
            if (!(value >= 0 && value <= 1)) {
                throw std::invalid_argument("ERROR: error should be between 0 and 1.");
            }
        }};

}  // namespace algos::config
