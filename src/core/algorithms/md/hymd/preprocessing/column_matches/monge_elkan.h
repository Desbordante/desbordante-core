#pragma once

#include <string>
#include <utility>
#include <vector>

#include "algorithms/md/hymd/preprocessing/column_matches/pairwise.h"

namespace algos::hymd::preprocessing::column_matches {
namespace similarity_measures {
double MongeElkan(std::vector<std::string> const& left, std::vector<std::string> const& right,
                  auto&& similarity_function) {
    if (left.empty() && right.empty()) return 1.0;
    if (left.empty() || right.empty()) return 0.0;
    // NOTE: for equivalence with Metanome, use the previous SWG implementation and set this to
    // float.
    using FloatingPointType = double;

    auto monge_elkan = [&](std::vector<std::string> const& left,
                           std::vector<std::string> const& right) {
        FloatingPointType sum = 0.0;
        for (std::string const& s : left) {
            auto right_it = right.begin();
            FloatingPointType max_sim = similarity_function(s, *right_it);
            for (auto const right_end = right.end(); ++right_it != right_end;) {
                FloatingPointType const similarity = similarity_function(s, *right_it);
                if (similarity > max_sim) max_sim = similarity;
            }
            sum += max_sim;
        }
        return sum / left.size();
    };

    return std::sqrt(monge_elkan(left, right) * monge_elkan(right, left));
}

double MongeElkan(std::vector<std::string> const& a, std::vector<std::string> const& b);
double MongeElkanString(std::string const& a, std::string const& b);
}  // namespace similarity_measures

class MongeElkan : public NormalPairwise<similarity_measures::MongeElkanString> {
    static constexpr auto kName = "monge_elkan";

public:
    template <typename... Args>
    MongeElkan(Args&&... args)
        : NormalPairwise<similarity_measures::MongeElkanString>(kName,
                                                                std::forward<Args>(args)...) {}
};
}  // namespace algos::hymd::preprocessing::column_matches
