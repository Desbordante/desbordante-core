#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_metric.h"

#include <algorithm>

template <typename SimilarityFunction>
double MongeElkan(std::vector<std::string> const& a, std::vector<std::string> const& b,
                  SimilarityFunction similarityFunction) {
    if (a.empty() && b.empty()) return 1.0;
    double sum = 0.0;
    for (auto const& s : a) {
        double max_sim = 0.0;
        for (auto const& q : b) {
            double similarity = similarityFunction(s, q);
            max_sim = std::max(max_sim, similarity);
        }
        sum += max_sim;
    }

    return a.empty() ? 0 : sum / a.size();
}

double MongeElkan(std::vector<std::string> const& a, std::vector<std::string> const& b) {
    return MongeElkan(a, b, [](std::string const& s1, std::string const& s2) {
        return NormalizedSmithWatermanGotoh(s1, s2);
    });
}