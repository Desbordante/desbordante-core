#include "core/algorithms/md/md_verifier/similarities/similarities.h"

namespace algos::md {
class EuclideanSimilarity : public AbstractSimilarityMeasure<long double> {
public:
    long double operator()(long double left, long double right) override;
};
}  // namespace algos::md