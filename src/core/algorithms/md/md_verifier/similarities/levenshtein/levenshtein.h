#include <string_view>

#include "core/algorithms/md/md_verifier/similarities/similarities.h"

namespace algos::md {
class LevenshteinSimilarity : public AbstractSimilarityMeasure<std::string_view&> {
public:
    long double operator()(std::string_view& left, std::string_view& right) override;
};
}  // namespace algos::md