#pragma once

#include <cmath>
#include <string>
#include <vector>

#include "algorithms/mde/decision_boundaries/float.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/pairwise.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/standard_calculator_creator.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/similarity.h"
#include "algorithms/mde/hymde/utility/compile_time_value.h"

namespace algos::hymde::record_match_indexes::calculators {
namespace monge_elkan_swg_word {
inline double MongeElkan(std::vector<std::string> const& left,
                         std::vector<std::string> const& right, auto&& similarity_function) {
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

inline double SmithWatermanGotoh(std::string const& s, std::string const& t,
                                 double gap_value = -0.5) {
    auto substitution_compare = [](char a, char b) { return a == b ? 1.0 : -2.0; };
    size_t m = s.size();
    size_t n = t.size();

    std::vector<double> v0(n, 0.0);
    std::vector<double> v1(n, 0.0);

    double max_score = 0.0;

    for (size_t j = 0; j < n; ++j) {
        v0[j] = std::max(0.0, j * gap_value + substitution_compare(s[0], t[j]));
        max_score = std::max(max_score, v0[j]);
    }

    for (size_t i = 1; i < m; ++i) {
        v1[0] = std::max({0.0, v0[0] + gap_value, substitution_compare(s[i], t[0])});

        max_score = std::max(max_score, v1[0]);

        for (size_t j = 1; j < n; ++j) {
            v1[j] = std::max({0.0, v0[j] + gap_value, v1[j - 1] + gap_value,
                              v0[j - 1] + substitution_compare(s[i], t[j])});
            max_score = std::max(max_score, v1[j]);
        }

        std::swap(v0, v1);
    }

    return max_score;
}

inline double NormalizedSmithWatermanGotoh(std::string const& a, std::string const& b,
                                           double gap_value = -0.5) {
    if (a.empty() && b.empty()) {
        return 1.0f;
    }
    if (a.empty() || b.empty()) {
        return 0.0f;
    }

    double max_distance = std::min(a.size(), b.size()) * std::max(1.0, gap_value);
    return SmithWatermanGotoh(a, b, gap_value) / max_distance;
}

inline double MongeElkan(std::vector<std::string> const& a, std::vector<std::string> const& b) {
    return MongeElkan(a, b, [](std::string const& s1, std::string const& s2) {
        return NormalizedSmithWatermanGotoh(s1, s2);
    });
}

inline std::vector<std::string> Tokenize(std::string const& text) {
    std::istringstream iss(text);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

inline double MongeElkanString(std::string const& a, std::string const& b) {
    return MongeElkan(Tokenize(a), Tokenize(b));
}

struct Traits {
    using Order = orders::Similarity;

    static constexpr auto kName = "monge_elkan";
    static constexpr Order::Type kEqValue = 1.0;
};
}  // namespace monge_elkan_swg_word

class MongeElkan
    : public NormalPairwise<monge_elkan_swg_word::Traits, monge_elkan_swg_word::MongeElkanString,
                            model::mde::decision_boundaries::Float,
                            utility::CompileTimeOptionalLike<1.0>> {
public:
    using Creator = StandardCalculatorCreator<MongeElkan>;

    using NormalPairwise<monge_elkan_swg_word::Traits, monge_elkan_swg_word::MongeElkanString,
                         model::mde::decision_boundaries::Float,
                         utility::CompileTimeOptionalLike<1.0>>::NormalPairwise;
};
}  // namespace algos::hymde::record_match_indexes::calculators
