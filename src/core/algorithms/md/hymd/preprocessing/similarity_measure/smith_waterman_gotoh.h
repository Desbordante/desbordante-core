#pragma once

#include <string>

namespace algos::hymd::preprocessing::similarity_measure {
float NormalizedSmithWatermanGotoh(std::string const& a, std::string const& b,
                                   float gap_value = -0.5f);
}  // namespace algos::hymd::preprocessing::similarity_measure
