#pragma once

#include <string>

namespace algos::hymd::preprocessing::similarity_measure {
size_t Lcs(std::string const& left, std::string const& right);
double LongestCommonSubsequence(std::string const& left, std::string const& right);
}  // namespace algos::hymd::preprocessing::similarity_measure
