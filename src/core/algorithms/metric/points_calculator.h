#pragma once

#include "core/algorithms/metric/points.h"
#include "core/config/indices/type.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/util/convex_hull.h"

namespace algos::metric {

class PointsCalculator {
private:
    bool dist_from_null_is_infinity_;
    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    config::IndicesType rhs_indices_;

    long double GetCoordinate(bool& has_values, ClusterIndex row_index, bool& has_nulls,
                              unsigned col_index, bool& has_empties) const;
    template <typename T>
    T GetPoint(ClusterIndex row_index, bool& has_values, bool& has_nulls, bool& has_empties,
               AssignmentFunction<T> const& assignment_func) const;

    long double GetDistFromNull() const {
        return dist_from_null_is_infinity_ ? std::numeric_limits<long double>::infinity() : 0.0;
    }

public:
    IndexedPointsCalculationResult<IndexedOneDimensionalPoint> CalculateIndexedPoints(
            model::PLI::Cluster const& cluster) const;

    IndexedPointsCalculationResult<IndexedVector> CalculateMultidimensionalIndexedPoints(
            model::PLI::Cluster const& cluster) const;

    template <typename T>
    PointsCalculationResult<T> CalculateMultidimensionalPoints(
            model::PLI::Cluster const& cluster, AssignmentFunction<T> const& assignment_func) const;

    PointsCalculationResult<util::Point> CalculateMultidimensionalPointsForCalipers(
            model::PLI::Cluster const& cluster) const;

    PointsCalculationResult<std::vector<long double>> CalculateMultidimensionalPointsForApprox(
            model::PLI::Cluster const& cluster) const;

    PointsCalculationResult<std::byte const*> CalculatePoints(
            model::PLI::Cluster const& cluster) const;

    explicit PointsCalculator(bool dist_from_null_is_infinity,
                              std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation,
                              config::IndicesType rhs_indices)
        : dist_from_null_is_infinity_(dist_from_null_is_infinity),
          typed_relation_(std::move(typed_relation)),
          rhs_indices_(std::move(rhs_indices)){};
};

}  // namespace algos::metric
