#include "core/algorithms/md/md_verifier/similarities/euclidean/euclidean.h"

#include "util/convex_hull.h"

namespace algos::md {
class EuclideanSimilarity : public AbstractSimilarityMeasure<long double> {
public:
    long double operator()(long double left, long double right) override {
        return 1 / (1 + util::EuclideanDistance({left}, {right}));
    }
};
}  // namespace algos::md