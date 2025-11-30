#pragma once

#include "core/algorithms/metric/highlight.h"

namespace algos::metric {

template <typename T>
struct IndexedPoint {
    T point;
    ClusterIndex index;
    IndexedPoint() = default;

    IndexedPoint(T const& point, ClusterIndex index) : point(point), index(index) {}

    IndexedPoint(IndexedPoint&& p) : point(std::move(p.point)), index(p.index) {}

    IndexedPoint& operator=(IndexedPoint&& p) {
        point = std::move(p.point);
        index = p.index;
        return *this;
    }
};

template <typename T>
struct IndexedPointsCalculationResult {
    std::vector<T> points;
    std::vector<Highlight> cluster_highlights;
    bool has_nulls;

    IndexedPointsCalculationResult(std::vector<T>&& points,
                                   std::vector<Highlight>&& cluster_highlights, bool has_nulls)
        : points(std::move(points)),
          cluster_highlights(std::move(cluster_highlights)),
          has_nulls(has_nulls) {}

    IndexedPointsCalculationResult(IndexedPointsCalculationResult&& other)
        : points(std::move(other.points)),
          cluster_highlights(std::move(other.cluster_highlights)),
          has_nulls(other.has_nulls) {}

    IndexedPointsCalculationResult& operator=(IndexedPointsCalculationResult&& other) {
        points = std::move(other.points);
        cluster_highlights = std::move(other.cluster_highlights);
        has_nulls = other.has_nulls;
        return *this;
    }
};

template <typename T>
struct PointsCalculationResult {
    std::vector<T> points;
    bool has_nulls;

    PointsCalculationResult(std::vector<T>&& points, bool has_nulls)
        : points(std::move(points)), has_nulls(has_nulls) {}

    PointsCalculationResult(PointsCalculationResult&& other)
        : points(std::move(other.points)), has_nulls(other.has_nulls) {}

    PointsCalculationResult& operator=(PointsCalculationResult&& other) {
        points = std::move(other.points);
        has_nulls = other.has_nulls;
        return *this;
    }
};

}  // namespace algos::metric
