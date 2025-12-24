#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos::metric {

enum class Metric : char {
    kEuclidean = 0, /* Standard metric for calculating the distance between
                     * numeric values */
    kLevenshtein,   /* Levenshtein distance between strings */
    kCosine         /* Represent strings as q-gram vectors and calculate cosine distance
                     * between these vectors */

};

enum class MetricAlgo : char {
    kBrute = 0, /* Brute force algorithm. Calculates distance between all possible pairs
                 * of values and compares them with parameter */
    kApprox,    /* 2-approximation linear time algorithm, which makes a prediction
                 * based on the max distance of one point in cluster */
    kCalipers   /* Rotating calipers algorithm for 2d euclidean metric. Computes a
                 * convex hull of the points and calculates distances between
                 * antipodal pairs of convex hull, resulting in a significant
                 * reduction in the number of comparisons */

};
}  // namespace algos::metric
