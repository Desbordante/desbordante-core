#pragma once

#include <string>

namespace algos::hymd::preprocessing::similarity_measure {
double NormalizedSmithWatermanGotoh(std::string const& s, std::string const& t,
                                    double gapValue = -0.5);
}  // namespace algos::hymd::preprocessing::similarity_measure
