#include "algorithms/md/hymd/preprocessing/similarity_measure/jaccard_metric.h"

#include <unordered_set>

double JaccardIndex(std::string const& word1, std::string const& word2) {
    std::unordered_set<char> set1(word1.begin(), word1.end());
    std::unordered_set<char> set2(word2.begin(), word2.end());

    int intersection_size = 0;
    for (auto c : set1) {
        if (set2.find(c) != set2.end()) {
            ++intersection_size;
        }
    }
    double union_size = static_cast<double>(set1.size() + set2.size() - intersection_size);

    return union_size == 0 ? 1.0 : intersection_size / union_size;
}