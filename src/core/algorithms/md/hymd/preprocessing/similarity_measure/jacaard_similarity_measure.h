#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"

double JacaardMetric(const std::byte* word1, const std::byte* word2) {
    std::string str1(reinterpret_cast<const char*>(word1));
    std::string str2(reinterpret_cast<const char*>(word2));

    std::unordered_set<char> set1(str1.begin(), str1.end());
    std::unordered_set<char> set2(str2.begin(), str2.end());

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
                                         Similarity sim = JacaardMetric(l, r);
                                         if (sim < min_sim) return kLowestBound;
                                         return sim;
                                     }) {}
};
}// namespace algos::hymd::preprocessing::similarity_measure