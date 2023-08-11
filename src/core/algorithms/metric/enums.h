#pragma once

#include <enum.h>

namespace algos::metric {

BETTER_ENUM(Metric, char,
    euclidean = 0,  /* Standard metric for calculating the distance between numeric
                     * values */
    levenshtein,    /* Levenshtein distance between strings */
    cosine          /* Represent strings as q-gram vectors and calculate cosine distance
                     * between these vectors */
)

BETTER_ENUM(MetricAlgo, char,
    brute = 0,      /* Brute force algorithm. Calculates distance between all possible pairs
                     * of values and compares them with parameter */
    approx,         /* 2-approximation linear time algorithm, which makes a prediction
                     * based on the max distance of one point in cluster */
    calipers        /* Rotating calipers algorithm for 2d euclidean metric. Computes a
                     * convex hull of the points and calculates distances between
                     * antipodal pairs of convex hull, resulting in a significant
                     * reduction in the number of comparisons */
)

}  // namespace algos::metric
