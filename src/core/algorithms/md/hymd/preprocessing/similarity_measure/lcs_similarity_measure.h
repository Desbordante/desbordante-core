#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"

double longest_common_subsequence(const std::byte* word1, const std::byte* word2) {
    std::string str1(reinterpret_cast<const char*>(word1));
    std::string str2(reinterpret_cast<const char*>(word2));

    size_t m = str1.size();
    size_t n = str2.size();

    std::vector<std::vector<size_t>> lcsTable(m + 1, std::vector<size_t>(n + 1, 0));

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (str1[i - 1] == str2[j - 1]) {
                lcsTable[i][j] = lcsTable[i - 1][j - 1] + 1;
            } else {
                lcsTable[i][j] = std::max(lcsTable[i - 1][j], lcsTable[i][j - 1]);
            }
        }
    }

    return lcsTable[m][n];
}


namespace algos::hymd::preprocessing::similarity_measure {

class LcsSimilarityMeasure : public ImmediateSimilarityMeasure {
private:
    model::md::DecisionBoundary min_sim;
public:
    LcsSimilarityMeasure(std::unique_ptr<model::Type> arg_type, double min_sim)
        : ImmediateSimilarityMeasure(std::move(arg_type),
                                     [min_sim](std::byte const* l, std::byte const* r) {
                                         std::string const& left = model::Type::GetValue<model::String>(l);
                                         std::string const& right = model::Type::GetValue<model::String>(r);
                                         std::size_t dist = longest_common_subsequence(l, r);
                                         std::size_t const max_dist = std::max(left.size(), right.size());
                                         Similarity sim = static_cast<double>(max_dist - dist) / static_cast<double>(max_dist);
                                         if (sim < min_sim) return kLowestBound;
                                         return sim;
                                     }) {}
};
}// namespace algos::hymd::preprocessing::similarity_measure