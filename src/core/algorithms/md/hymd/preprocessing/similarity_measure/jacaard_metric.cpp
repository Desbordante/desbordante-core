#include "algorithms/md/hymd/preprocessing/similarity_measure/jacaard_metric.h"

#include <unordered_set>

double JacaardMetric(std::string const& word1, std::string const& word2) {
    std::unordered_set<char> set1(word1.begin(), word1.end());
    std::unordered_set<char> set2(word2.begin(), word2.end());

    std::unordered_set<char> intersection;
    for (char ch : set1) {
        if (set2.find(ch) != set2.end()) {
            intersection.insert(ch);
        }
    }

    double intersection_size = static_cast<double>(intersection.size());
    double union_size = static_cast<double>(set1.size() + set2.size() - intersection_size);

    return union_size == 0 ? 1.0 : 1 - intersection_size / union_size;
}