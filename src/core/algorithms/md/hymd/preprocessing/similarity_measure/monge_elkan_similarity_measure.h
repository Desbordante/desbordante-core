#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/jacaard_similarity_measure.h"

double MongeElkan(const std::byte* word1, const std::byte* word2) {
    std::string str1(reinterpret_cast<const char*>(word1));
    std::string str2(reinterpret_cast<const char*>(word2));

    double cummax = 0.0;
    std::istringstream iss1(str1), iss2(str2);
    std::string token1, token2;

    while (std::getline(iss1, token1, ' ')) {
        double maxscore = 0.0;
        iss2.clear();
        iss2.seekg(0);

        while (std::getline(iss2, token2, ' ')) {
            maxscore = std::max(maxscore, JacaardMetric(reinterpret_cast<const std::byte*>(token1.c_str()), reinterpret_cast<const std::byte*>(token2.c_str())));
        }
        cummax += maxscore;
    }
    return cummax / std::count(str1.begin(), str1.end(), ' ') + 1;
}


namespace algos::hymd::preprocessing::similarity_measure {

class JacaardSimilarityMeasure : public ImmediateSimilarityMeasure {
private:
    model::md::DecisionBoundary min_sim;
public:
    JacaardSimilarityMeasure(std::unique_ptr<model::Type> arg_type, double min_sim)
        : ImmediateSimilarityMeasure(std::move(arg_type),
                                     [min_sim](std::byte const* l, std::byte const* r) {
                                         std::string const& left = model::Type::GetValue<model::String>(l);
                                         std::string const& right = model::Type::GetValue<model::String>(r);
                                         Similarity sim = MongeElkan(l, r);
                                         if (sim < min_sim) return kLowestBound;
                                         return sim;
                                     }) {}
};
}// namespace algos::hymd::preprocessing::similarity_measure