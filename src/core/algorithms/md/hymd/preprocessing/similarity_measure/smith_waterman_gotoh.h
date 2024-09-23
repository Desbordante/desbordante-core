#pragma once

#include <string>

namespace algos::hymd::preprocessing::similarity_measure {
double NormalizedSmithWatermanGotoh(std::string const& a, std::string const& b,
                                    double gap_value = -0.5);
}  // namespace algos::hymd::preprocessing::similarity_measure
