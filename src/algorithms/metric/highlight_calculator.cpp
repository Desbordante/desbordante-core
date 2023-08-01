#include "algorithms/metric/highlight_calculator.h"

#include <algorithm>

#include "util/convex_hull.h"

namespace {

void UpdateHighlightMap(algos::metric::HighlightMap& highlight_map,
                        algos::metric::ClusterIndex index,
                        algos::metric::ClusterIndex furthest_index, long double dist) {
    auto it = highlight_map.try_emplace(index, index, furthest_index, dist).first;
    if (it->second.max_distance < dist) {
        it->second.max_distance = dist;
        it->second.furthest_data_index = furthest_index;
    }
}

}  // namespace

namespace algos::metric {

void HighlightCalculator::CalculateOneDimensionalHighlights(
        std::vector<IndexedOneDimensionalPoint> const& indexed_points,
        std::vector<Highlight>&& cluster_highlights) {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
    auto const& type = static_cast<model::INumericType const&>(col.GetType());

    auto [min_value, max_value] = std::minmax_element(
            indexed_points.begin(), indexed_points.end(), [&type](auto const& a, auto const& b) {
                return type.Compare(a.point, b.point) == model::CompareResult::kLess;
            });

    for (auto const& indexed_point : indexed_points) {
        long double dist_to_max_element = type.Dist(indexed_point.point, max_value->point);
        long double dist_to_min_element = type.Dist(indexed_point.point, min_value->point);
        long double max_dist = 0;
        ClusterIndex furthest_point_index = 0;
        if (dist_to_max_element > dist_to_min_element) {
            max_dist = dist_to_max_element;
            furthest_point_index = max_value->index;
        } else {
            max_dist = dist_to_min_element;
            furthest_point_index = min_value->index;
        }
        cluster_highlights.emplace_back(indexed_point.index, furthest_point_index, max_dist);
    }
    highlights_.push_back(std::move(cluster_highlights));
}

template <typename T>
void HighlightCalculator::BruteCalculateHighlights(
        std::vector<IndexedPoint<T>> const& indexed_points,
        std::vector<Highlight>&& cluster_highlights, DistanceFunction<T> const& dist_func) {
    HighlightMap highlight_map;

    for (size_t i = 0; i + 1 < indexed_points.size(); ++i) {
        long double max_dist = 0;
        ClusterIndex furthest_point_index = indexed_points[i].index;
        for (size_t j = i + 1; j < indexed_points.size(); ++j) {
            long double dist = dist_func(indexed_points[i].point, indexed_points[j].point);
            if (dist > max_dist) {
                max_dist = dist;
                furthest_point_index = indexed_points[j].index;
            }
            UpdateHighlightMap(highlight_map, indexed_points[j].index, indexed_points[i].index,
                               dist);
        }
        UpdateHighlightMap(highlight_map, indexed_points[i].index, furthest_point_index, max_dist);
    }
    if (indexed_points.size() == 1) {
        cluster_highlights.emplace_back(indexed_points[0].index, indexed_points[0].index, 0);
    } else {
        for (auto const& pair : highlight_map) {
            cluster_highlights.push_back(pair.second);
        }
    }
    highlights_.push_back(std::move(cluster_highlights));
}

void HighlightCalculator::CalculateHighlightsForStrings(
        std::vector<IndexedPoint<std::byte const*>> const& indexed_points,
        std::vector<Highlight>&& cluster_highlights,
        DistanceFunction<std::byte const*> const& dist_func) {
    BruteCalculateHighlights(indexed_points, std::move(cluster_highlights), dist_func);
}

void HighlightCalculator::CalculateMultidimensionalHighlights(
        std::vector<IndexedPoint<std::vector<long double>>> const& indexed_points,
        std::vector<Highlight>&& cluster_highlights) {
    BruteCalculateHighlights<std::vector<long double>>(
            indexed_points, std::move(cluster_highlights), util::EuclideanDistance);
}

void HighlightCalculator::SortHighlightsByDistanceAscending() {
    SortHighlights([this](auto const& h1, auto const& h2) {
        auto const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
        if (col.IsEmpty(h1.data_index)) {
            return false;
        }
        if (col.IsEmpty(h2.data_index)) {
            return true;
        }
        if (h1.max_distance == 0 && h2.max_distance == 0) {
            if (col.IsNull(h1.data_index)) {
                return true;
            }
            if (col.IsNull(h2.data_index)) {
                return false;
            }
        }
        return h1.max_distance < h2.max_distance;
    });
}

void HighlightCalculator::SortHighlightsByDistanceDescending() {
    SortHighlights([this](auto const& h1, auto const& h2) {
        if (h1.max_distance == 0 && h2.max_distance == 0) {
            auto const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
            if (col.IsEmpty(h1.data_index)) {
                return false;
            }
            if (col.IsEmpty(h2.data_index)) {
                return true;
            }
            if (col.IsNull(h1.data_index)) {
                return false;
            }
            if (col.IsNull(h2.data_index)) {
                return true;
            }
        }
        return h1.max_distance > h2.max_distance;
    });
}

void HighlightCalculator::SortHighlightsByFurthestIndexAscending() {
    SortHighlights([this](auto const& h1, auto const& h2) {
        auto const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
        if (col.IsEmpty(h1.data_index)) {
            return false;
        }
        if (col.IsEmpty(h2.data_index)) {
            return true;
        }
        if (col.IsNull(h1.data_index)) {
            return false;
        }
        if (col.IsNull(h2.data_index)) {
            return true;
        }
        return h1.furthest_data_index < h2.furthest_data_index;
    });
}

void HighlightCalculator::SortHighlightsByFurthestIndexDescending() {
    SortHighlights([this](auto const& h1, auto const& h2) {
        auto const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
        if (col.IsEmpty(h1.data_index)) {
            return false;
        }
        if (col.IsEmpty(h2.data_index)) {
            return true;
        }
        if (col.IsNull(h1.data_index)) {
            return false;
        }
        if (col.IsNull(h2.data_index)) {
            return true;
        }
        return h1.furthest_data_index > h2.furthest_data_index;
    });
}

void HighlightCalculator::SortHighlightsByIndexAscending() {
    SortHighlights([](auto const& h1, auto const& h2) { return h1.data_index < h2.data_index; });
}

void HighlightCalculator::SortHighlightsByIndexDescending() {
    SortHighlights([](auto const& h1, auto const& h2) { return h1.data_index > h2.data_index; });
}

}  // namespace algos::metric
