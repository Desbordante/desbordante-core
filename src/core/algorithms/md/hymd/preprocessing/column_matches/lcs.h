#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include "algorithms/md/hymd/preprocessing/column_matches/pairwise.h"
#include "config/exceptions.h"
#include "model/types/string_type.h"

namespace algos::hymd::preprocessing::column_matches {
namespace similarity_measures {
std::size_t Lcs(std::string const& left, std::string const& right);
double LongestCommonSubsequence(std::string const& left, std::string const& right);
}  // namespace similarity_measures

class Lcs : public NormalPairwise<similarity_measures::LongestCommonSubsequence> {
    static constexpr auto kName = "lcs";

public:
    template <typename... Args>
    Lcs(Args&&... args)
        : NormalPairwise<similarity_measures::LongestCommonSubsequence>(
                  kName, std::forward<Args>(args)...) {}
};
}  // namespace algos::hymd::preprocessing::column_matches
