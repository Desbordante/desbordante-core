#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "util/position_list_index.h"

namespace algos::metric {

template <typename T>
struct IndexedPoint;
template <typename T>
struct IndexedPointsCalculationResult;
template <typename T>
struct PointsCalculationResult;
struct Highlight;

using ClusterIndex = util::PLI::Cluster::value_type;

using IndexedOneDimensionalPoint = IndexedPoint<std::byte const*>;
using IndexedVector = IndexedPoint<std::vector<long double>>;

template <typename T>
using DistanceFunction = std::function<long double(T, T)>;
template <typename T>
using CompareFunction = std::function<bool(std::vector<T> const& points)>;
template <typename T>
using HighlightFunction = std::function<void(std::vector<T> const& points,
                                             std::vector<Highlight>&& cluster_highlights)>;
using ClusterFunction = std::function<bool(util::PLI::Cluster const& cluster)>;
template <typename T>
using IndexedPointsFunction =
        std::function<IndexedPointsCalculationResult<T>(util::PLI::Cluster const& cluster)>;
template <typename T>
using PointsFunction = std::function<PointsCalculationResult<T>(util::PLI::Cluster const& cluster)>;
template <typename T>
using AssignmentFunction = std::function<void(long double, T&, size_t)>;

using HighlightMap = std::unordered_map<ClusterIndex, Highlight>;

}  // namespace algos::metric
