#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_metric.h"

#include <algorithm>
#include <vector>

#include "algorithms/md/hymd/preprocessing/similarity_measure/jaccard_similarity_measure.h"

double MongeElkan(std::string const& word1, std::string const& word2) {
    double cummax = 0.0;
    std::vector<std::string> words1;
    std::vector<std::string> words2;

    size_t start = 0;
    size_t end = 0;
    while ((end = word1.find(" ", start)) != std::string::npos) {
        words1.push_back(word1.substr(start, end - start));
        start = end + 1;
    }
    words1.push_back(word1.substr(start));

    start = 0;
    while ((end = word2.find(" ", start)) != std::string::npos) {
        words2.push_back(word2.substr(start, end - start));
        start = end + 1;
    }
    words2.push_back(word2.substr(start));

    for (auto const& w1 : words1) {
        double maxscore = 0.0;
        for (auto const& w2 : words2) {
            maxscore = std::max(maxscore, JaccardIndex(w1, w2));
        }
        cummax += maxscore;
    }

    return cummax / words1.size();
}