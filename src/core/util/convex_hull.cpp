#include "convex_hull.h"

#include <algorithm>
#include <cstddef>

namespace util {

// Returns cross product of vectors p0p1 and p0p2
static long double CrossProduct(Point const& p0, Point const& p1, Point const& p2) {
    return (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
}

std::vector<std::pair<Point, Point>> GetAntipodalPairs(std::vector<Point> const& conv_hull) {
    size_t n = conv_hull.size();
    std::vector<std::pair<Point, Point>> antipodal_pairs;
    if (n <= 1) return antipodal_pairs;
    if (n == 2) {
        antipodal_pairs.emplace_back(conv_hull[0], conv_hull[1]);
        return antipodal_pairs;
    }
    auto next = [n](size_t p) { return (p + 1) % n; };
    auto area = [&conv_hull](size_t a, size_t b, size_t c) {
        return CrossProduct(conv_hull[a], conv_hull[b], conv_hull[c]);
    };
    size_t p = n - 1;
    size_t next_p = 0;
    size_t q = 0;
    while (area(p, next_p, next(q)) > area(p, next_p, q)) {
        q = next(q);
    }
    size_t q0 = q;
    while (q != 0) {
        p = next_p;
        next_p = next(p);
        antipodal_pairs.emplace_back(conv_hull[p], conv_hull[q]);
        long double a = area(p, next_p, next(q));
        long double b = area(p, next_p, q);
        while (a > b) {
            q = next(q);
            if (p != q0 && q != 0) {
                antipodal_pairs.emplace_back(conv_hull[p], conv_hull[q]);
            } else {
                return antipodal_pairs;
            }
            a = area(p, next_p, next(q));
            b = area(p, next_p, q);
        }
        if (a == b) {
            q = next(q);
        }
    }
    return antipodal_pairs;
}

std::vector<Point> CalculateConvexHull(std::vector<Point>& points) {
    std::sort(points.begin(), points.end(),
              [](Point& p1, Point& p2) { return (p1.x < p2.x || (p1.x == p2.x && p1.y < p2.y)); });
    points.erase(std::unique(points.begin(), points.end(),
                             [](Point const& p1, Point const& p2) {
                                 return p1.x == p2.x && p1.y == p2.y;
                             }),
                 points.end());

    int n = (int)points.size();
    if (n < 3) return points;

    std::vector<Point> result(2 * n);
    int k = 0;

    for (int i = 0; i < n; ++i) {
        while (k >= 2 && CrossProduct(result[k - 2], result[k - 1], points[i]) <= 0) k--;
        result[k++] = points[i];
    }

    for (int i = n - 2, t = k + 1; i >= 0; --i) {
        while (k >= t && CrossProduct(result[k - 2], result[k - 1], points[i]) <= 0) k--;
        result[k++] = points[i];
    }

    result.resize(k - 1);
    return result;
}

}  // namespace util
