#pragma once

#include <string>  // for string
#include <vector>  // for vector

class Vertical;

namespace algos::nd::util {

[[nodiscard]] std::vector<std::string> GetVerticalNames(Vertical const& vert);

}  // namespace algos::nd::util
