#pragma once

#include <cassert>
#include <cmath>
#include <functional>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>

namespace util {

struct Point {
    long double x;
    long double y;

    static long double EuclideanDistance(util::Point const& p1, util::Point const& p2) {
        return std::sqrt(std::pow((p1.x - p2.x), 2) + std::pow((p1.y - p2.y), 2));
    }
};

[[maybe_unused]] static long double EuclideanDistance(std::vector<long double> const& p1,
                                                      std::vector<long double> const& p2) {
    assert(p1.size() == p2.size());
    assert(!p1.empty());
    return std::sqrt(
            std::inner_product(p1.cbegin(), p1.cend(), p2.cbegin(), 0.0, std::plus<>(),
                               [](long double a, long double b) { return (a - b) * (a - b); }));
}

/** Monotone chain convex hull algorithm.
 * https://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain
 * @param points Vector of points, which can be modified as a result of the algorithm.
 * @return Convex hull sorted in counterclockwise order.
 */
std::vector<Point> CalculateConvexHull(std::vector<Point>& points);

/** Rotating calipers algorithm used to obtain antipodal pairs of convex hull
 * https://en.wikipedia.org/wiki/Rotating_calipers
 * @param conv_hull Convex hull that must be sorted in counterclockwise order.
 * @return Vector of antipodal pairs. The algorithm does not return all antipodal pairs, but only
 * those that are more likely to form the diameter of the convex hull.
 */
std::vector<std::pair<Point, Point>> GetAntipodalPairs(std::vector<Point> const& conv_hull);

}  // namespace util
