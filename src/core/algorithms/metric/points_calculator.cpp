#include "algorithms/metric/points_calculator.h"

#include <stdexcept>

#include "builtin.h"
#include "convex_hull.h"
#include "metric/highlight.h"
#include "metric/points.h"
#include "table/column_layout_typed_relation_data.h"
#include "table/typed_column_data.h"
#include "type.h"

namespace {

void AssignToVector(long double coord, std::vector<long double>& point, [[maybe_unused]] size_t j) {
    point.push_back(coord);
}

void AssignToPoint(long double coord, util::Point& point, size_t j) {
    if (j == 0) {
        point.x = coord;
    } else {
        point.y = coord;
    }
}

void AssignToIndexedVector(long double coord, algos::metric::IndexedVector& point,
                           [[maybe_unused]] size_t j) {
    point.point.push_back(coord);
}

}  // namespace

namespace algos::metric {

long double PointsCalculator::GetCoordinate(bool& has_values, ClusterIndex row_index,
                                            bool& has_nulls, unsigned col_index,
                                            bool& has_empties) const {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(col_index);

    if (col.IsNull(row_index)) {
        if (has_values) {
            throw std::runtime_error("Some of the value coordinates are nulls.");
        }
        has_nulls = true;
        return 0;
    }
    if (col.IsEmpty(row_index)) {
        if (has_values) {
            throw std::runtime_error("Some of the value coordinates are empties.");
        }
        has_empties = true;
        return 0;
    }
    if (has_nulls || has_empties) {
        throw std::runtime_error("Some of the value coordinates are nulls or empties.");
    }

    has_values = true;
    return col.GetType().GetTypeId() == +model::TypeId::kInt
                   ? (long double)model::Type::GetValue<model::Int>(col.GetData()[row_index])
                   : model::Type::GetValue<model::Double>(col.GetData()[row_index]);
}

template <typename T>
T PointsCalculator::GetPoint(ClusterIndex row_index, bool& has_values, bool& has_nulls,
                             bool& has_empties,
                             AssignmentFunction<T> const& assignment_func) const {
    T point = {};
    for (size_t j = 0; j < rhs_indices_.size(); ++j) {
        long double coord =
                GetCoordinate(has_values, row_index, has_nulls, rhs_indices_[j], has_empties);
        if (!has_values) {
            continue;
        }
        assignment_func(coord, point, j);
    }
    return point;
}

IndexedPointsCalculationResult<IndexedVector>
PointsCalculator::CalculateMultidimensionalIndexedPoints(model::PLI::Cluster const& cluster) const {
    std::vector<IndexedVector> points;
    std::vector<Highlight> cluster_highlights;
    bool has_nulls_in_cluster = false;
    for (auto i : cluster) {
        bool has_values = false;
        bool has_nulls = false;
        bool has_empties = false;
        auto point = GetPoint<IndexedVector>(i, has_values, has_nulls, has_empties,
                                             AssignToIndexedVector);
        if (has_values) {
            point.index = i;
            points.push_back(std::move(point));
        } else if (has_nulls) {
            has_nulls_in_cluster = true;
            cluster_highlights.emplace_back(i, i, GetDistFromNull());
        } else if (has_empties) {
            cluster_highlights.emplace_back(i, i, 0.0);
        }
    }
    return {std::move(points), std::move(cluster_highlights), has_nulls_in_cluster};
}

IndexedPointsCalculationResult<IndexedOneDimensionalPoint> PointsCalculator::CalculateIndexedPoints(
        model::PLI::Cluster const& cluster) const {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
    std::vector<std::byte const*> const& data = col.GetData();
    std::vector<IndexedPoint<std::byte const*>> points;
    std::vector<Highlight> cluster_highlights;
    bool has_nulls_in_cluster = false;
    for (auto i : cluster) {
        if (col.IsNull(i)) {
            has_nulls_in_cluster = true;
            cluster_highlights.emplace_back(i, i, GetDistFromNull());
            continue;
        }
        if (col.IsEmpty(i)) {
            cluster_highlights.emplace_back(i, i, 0.0);
            continue;
        }
        points.emplace_back(data[i], i);
    }
    return {std::move(points), std::move(cluster_highlights), has_nulls_in_cluster};
}

template <typename T>
PointsCalculationResult<T> PointsCalculator::CalculateMultidimensionalPoints(
        model::PLI::Cluster const& cluster, AssignmentFunction<T> const& assignment_func) const {
    std::vector<T> points;
    bool has_nulls_in_cluster = false;
    for (auto i : cluster) {
        bool has_values = false;
        bool has_nulls = false;
        bool has_empties = false;
        T point = GetPoint(i, has_values, has_nulls, has_empties, assignment_func);
        if (dist_from_null_is_infinity_ && has_nulls) {
            return {{}, true};
        }
        has_nulls_in_cluster |= has_nulls;
        if (has_values) {
            points.push_back(std::move(point));
        }
    }
    return {std::move(points), has_nulls_in_cluster};
}

PointsCalculationResult<util::Point> PointsCalculator::CalculateMultidimensionalPointsForCalipers(
        model::PLI::Cluster const& cluster) const {
    return CalculateMultidimensionalPoints<util::Point>(cluster, AssignToPoint);
}

PointsCalculationResult<std::vector<long double>>
PointsCalculator::CalculateMultidimensionalPointsForApprox(
        model::PLI::Cluster const& cluster) const {
    return CalculateMultidimensionalPoints<std::vector<long double>>(cluster, AssignToVector);
}

PointsCalculationResult<std::byte const*> PointsCalculator::CalculatePoints(
        model::PLI::Cluster const& cluster) const {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
    std::vector<std::byte const*> const& data = col.GetData();
    std::vector<std::byte const*> points;
    bool has_nulls_in_cluster = false;
    for (auto i : cluster) {
        if (col.IsNull(i)) {
            if (dist_from_null_is_infinity_) {
                return {{}, true};
            }
            has_nulls_in_cluster = true;
            continue;
        }
        if (col.IsEmpty(i)) {
            continue;
        }
        points.emplace_back(data[i]);
    }
    return {std::move(points), has_nulls_in_cluster};
}

}  // namespace algos::metric
