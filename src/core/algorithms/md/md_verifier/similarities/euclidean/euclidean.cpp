#include "algorithms/md/md_verifier/similarities/euclidean/euclidean.h"

#include "util/convex_hull.h"

namespace algos::md {

long double EuclideanSimilarity::operator()(long double left, long double right) {
    return 1.0 / (1.0 + util::EuclideanDistance({left}, {right}));
}

}  // namespace algos::md