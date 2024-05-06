#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_metric.h"

#include <algorithm>
#include <iterator>
#include <sstream>

#include "algorithms/md/hymd/preprocessing/similarity_measure/jacaard_similarity_measure.h"

double MongeElkan(std::string const& word1, std::string const& word2) {
    double cummax = 0.0;
    std::istringstream iss1(word1), iss2(word2);
    std::string token1, token2;

    while (std::getline(iss1, token1, ' ')) {
        double maxscore = 0.0;
        iss2.clear();
        iss2.seekg(0);

        while (std::getline(iss2, token2, ' ')) {
            maxscore = std::max(maxscore, JacaardMetric(token1.c_str(), token2.c_str()));
        }
        cummax += maxscore;
    }
    return cummax / std::count(word1.begin(), word1.end(), ' ') + 1;
}