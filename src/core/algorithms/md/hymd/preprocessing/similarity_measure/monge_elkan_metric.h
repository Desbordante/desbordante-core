#include <functional>
#include <string>
#include <vector>

#include "algorithms/md/hymd/preprocessing/similarity_measure/smith_waterman_gotoh.h"
template <typename SimilarityFunction>
double MongeElkan(std::vector<std::string> const& a, std::vector<std::string> const& b,
                  SimilarityFunction similarityFunction);

double MongeElkan(std::vector<std::string> const& a, std::vector<std::string> const& b);