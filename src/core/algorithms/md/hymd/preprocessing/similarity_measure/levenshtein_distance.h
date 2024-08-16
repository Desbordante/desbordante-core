#pragma once

#include <string>

namespace algos::hymd::preprocessing::similarity_measure {
double LevenshteinDistance(std::string const& l, std::string const& r) noexcept;
}  // namespace algos::hymd::preprocessing::similarity_measure
