#pragma once

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "algorithms/metric_verifier_enums.h"
#include "algorithms/options/equal_nulls_opt.h"
#include "algorithms/primitive.h"
#include "model/column_layout_relation_data.h"
#include "model/column_layout_typed_relation_data.h"
#include "util/convex_hull.h"
#include "util/qgram_vector.h"

namespace algos {

class MetricVerifier : public algos::Primitive {
private:
    using ClusterIndex = util::PLI::Cluster::value_type;

public:
    struct Highlight {
        ClusterIndex data_index;
        ClusterIndex furthest_data_index;
        long double max_distance;

        Highlight(ClusterIndex data_index, ClusterIndex furthest_data_index,
                  long double max_distance)
            : data_index(data_index),
              furthest_data_index(furthest_data_index),
              max_distance(max_distance) {}
    };

private:
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
    using PointsFunction =
            std::function<PointsCalculationResult<T>(util::PLI::Cluster const& cluster)>;
    template <typename T>
    using AssignmentFunction = std::function<void(long double, T&, size_t)>;

    static void AssignToVector(long double coord, std::vector<long double>& point,
                               [[maybe_unused]] size_t j);

    static void AssignToIndexedVector(long double coord, IndexedVector& point,
                                      [[maybe_unused]] size_t j);

    static void AssignToPoint(long double coord, util::Point& point, size_t j);

    using HighlightMap = std::unordered_map<ClusterIndex, Highlight>;

    static void UpdateHighlightMap(HighlightMap& highlight_map, ClusterIndex index,
                                   ClusterIndex furthest_index, long double dist);

    Metric metric_ = Metric::_values()[0];
    MetricAlgo algo_ = MetricAlgo::_values()[0];
    std::vector<unsigned int> lhs_indices_;
    std::vector<unsigned int> rhs_indices_;
    long double parameter_;
    unsigned int q_;
    bool dist_from_null_is_infinity_;
    config::EqNullsType is_null_equal_null_;

    bool metric_fd_holds_ = false;
    std::vector<std::vector<Highlight>> highlights_;

    static const config::OptionType<decltype(dist_from_null_is_infinity_)>
            DistFromNullIsInfinityOpt;
    static const config::OptionType<decltype(parameter_)> ParameterOpt;
    static const config::OptionType<decltype(lhs_indices_)> LhsIndicesOpt;
    static const config::OptionType<decltype(rhs_indices_)> RhsIndicesOpt;
    static const config::OptionType<decltype(metric_)> MetricOpt;
    static const config::OptionType<decltype(algo_)> AlgoOpt;
    static const config::OptionType<decltype(q_)> QGramLengthOpt;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::shared_ptr<ColumnLayoutRelationData> relation_;  // temporarily parsing twice

    DistanceFunction<std::byte const*> GetCosineDistFunction(
            model::StringType const& type,
            std::unordered_map<std::string, util::QGramVector>& q_gram_map) const;

    bool CheckMFDFailIfHasNulls(bool has_nulls) const {
        return dist_from_null_is_infinity_ && has_nulls;
    }
    long double GetDistFromNull() const {
        return dist_from_null_is_infinity_ ? std::numeric_limits<long double>::infinity() : 0.0;
    }
    long double GetCoordinate(bool& has_values, ClusterIndex row_index, bool& has_nulls,
                              unsigned col_index, bool& has_empties) const;
    template <typename T>
    T GetPoint(ClusterIndex row_index, bool& has_values, bool& has_nulls, bool& has_empties,
               AssignmentFunction<T> const& assignment_func) const;
    IndexedPointsCalculationResult<IndexedOneDimensionalPoint> CalculateIndexedPoints(
            util::PLI::Cluster const& cluster) const;
    IndexedPointsCalculationResult<IndexedVector> CalculateMultidimensionalIndexedPoints(
            util::PLI::Cluster const& cluster) const;
    template <typename T>
    PointsCalculationResult<T> CalculateMultidimensionalPoints(
            util::PLI::Cluster const& cluster, AssignmentFunction<T> const& assignment_func) const;
    PointsCalculationResult<util::Point> CalculateMultidimensionalPointsForCalipers(
            util::PLI::Cluster const& cluster) const {
        return CalculateMultidimensionalPoints<util::Point>(cluster, AssignToPoint);
    }
    PointsCalculationResult<std::vector<long double>> CalculateMultidimensionalPointsForApprox(
            util::PLI::Cluster const& cluster) const {
        return CalculateMultidimensionalPoints<std::vector<long double>>(cluster, AssignToVector);
    }
    PointsCalculationResult<std::byte const*> CalculatePoints(
            util::PLI::Cluster const& cluster) const;

    bool CompareNumericValues(std::vector<IndexedOneDimensionalPoint> const& points) const;
    void CalculateOneDimensionalHighlights(
            std::vector<IndexedOneDimensionalPoint> const& indexed_points,
            std::vector<Highlight>&& cluster_highlights);
    template <typename T>
    bool ApproxVerifyCluster(std::vector<T> const& points,
                             DistanceFunction<T> const& dist_func) const;
    template <typename T>
    bool BruteVerifyCluster(std::vector<IndexedPoint<T>> const& points,
                            DistanceFunction<T> const& dist_func) const;
    template <typename T>
    void BruteCalculateHighlights(std::vector<IndexedPoint<T>> const& indexed_points,
                                  std::vector<Highlight>&& cluster_highlights,
                                  DistanceFunction<T> const& dist_func);

    bool CalipersCompareNumericValues(std::vector<util::Point>& points) const;
    template <typename T>
    ClusterFunction CalculateClusterFunction(IndexedPointsFunction<T> points_func,
                                             CompareFunction<T> compare_func,
                                             HighlightFunction<T> highlight_func) const;
    template <typename T>
    ClusterFunction CalculateApproxClusterFunction(PointsFunction<T> points_func,
                                                   DistanceFunction<T> dist_func) const;
    ClusterFunction GetClusterFunctionForSeveralDimensions();
    ClusterFunction GetClusterFunctionForOneDimension();
    ClusterFunction GetClusterFunction();
    void VerifyMetricFD();
    std::string GetStringValue(std::vector<unsigned> const& index_vec,
                               ClusterIndex row_index) const;
    void VisualizeHighlights() const;
    static_assert(std::is_same<decltype(MetricVerifier::lhs_indices_),
            decltype(MetricVerifier::rhs_indices_)>{}, "Types of indices must be the same");
    void ValidateIndices(decltype(MetricVerifier::lhs_indices_) const& indices) const;
    void ValidateRhs(decltype(MetricVerifier::rhs_indices_) const& indices);
    void RegisterOptions();
    template <typename Compare>
    void SortHighlights(Compare compare) {
        for (auto& cluster_highlight : highlights_) {
            std::sort(cluster_highlight.begin(), cluster_highlight.end(), compare);
        }
    }

protected:
    void FitInternal(model::IDatasetStream &data_stream) override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    bool GetResult() const {
        return metric_fd_holds_;
    }

    std::vector<std::vector<Highlight>> const& GetHighlights() const {
        return highlights_;
    }

    void SetParameter(long double parameter) {
        parameter_ = parameter;
    }

    void SortHighlightsByDistance() {
        SortHighlights(
                [](auto const& h1, auto const& h2) { return h1.max_distance > h2.max_distance; });
    }
    void SortHighlightsByIndex() {
        SortHighlights(
                [](auto const& h1, auto const& h2) { return h1.data_index < h2.data_index; });
    }

    MetricVerifier();
};

}  // namespace algos
