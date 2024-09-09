#pragma once

#include <string>
#include <vector>

#include "model/table/vertical.h"

namespace algos::nd::util {

[[nodiscard]] std::vector<std::string> GetVerticalNames(Vertical const& vert);

}  // namespace algos::nd::util
