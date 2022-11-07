#include "algorithms/metric_verifier.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

#include <easylogging++.h>

namespace algos {

void TransformIndices(std::vector<unsigned int>& value) {
    if (value.empty()) {
        throw std::invalid_argument("Indices cannot be empty");
    }
    std::sort(value.begin(), value.end());
    value.erase(std::unique(value.begin(), value.end()), value.end());
}

decltype(MetricVerifier::DistToNullInfinityOpt) MetricVerifier::DistToNullInfinityOpt{
        {config::names::kDistToNullIsInfinity, config::descriptions::kDDistToNullIsInfinity}, false
};

decltype(MetricVerifier::ParameterOpt) MetricVerifier::ParameterOpt{
        {config::names::kParameter, config::descriptions::kDParameter}, {}, [](auto value) {
            if (value < 0) throw std::invalid_argument("Parameter out of range");
        }
};

decltype(MetricVerifier::LhsIndicesOpt) MetricVerifier::LhsIndicesOpt{
        {config::names::kLhsIndices, config::descriptions::kDLhsIndices}, {}, TransformIndices
};

decltype(MetricVerifier::RhsIndicesOpt) MetricVerifier::RhsIndicesOpt{
        {config::names::kRhsIndices, config::descriptions::kDRhsIndices}, {}, TransformIndices
};

decltype(MetricVerifier::MetricOpt) MetricVerifier::MetricOpt{
        {config::names::kMetric, config::descriptions::kDMetric}
};

decltype(MetricVerifier::AlgoOpt) MetricVerifier::AlgoOpt{
        {config::names::kMetricAlgorithm, config::descriptions::kDMetricAlgorithm}
};

decltype(MetricVerifier::QGramLengthOpt) MetricVerifier::QGramLengthOpt{
        {config::names::kQGramLength, config::descriptions::kDQGramLength}, 2, [](auto value) {
            if (value <= 0)
                throw std::invalid_argument("Q-gram length should be greater than zero.");
        }
};

MetricVerifier::MetricVerifier() : Primitive({}) {
    RegisterOptions();
    MakeOptionsAvailable(config::GetOptionNames(config::EqualNullsOpt));
}

void MetricVerifier::ValidateIndices(decltype(MetricVerifier::lhs_indices_) const& value) const {
    size_t cols_count = relation_->GetSchema()->GetNumColumns();
    auto value_out_of_range = [cols_count](unsigned int i) { return i >= cols_count; };
    if (std::any_of(value.begin(), value.end(), value_out_of_range)) {
        throw std::runtime_error(
                "Column index should be less than the number of columns in the dataset.");
    }
}

void MetricVerifier::ValidateRhs(decltype(MetricVerifier::rhs_indices_) const& value) {
    ValidateIndices(value);
    if (value.size() == 1) {
        auto column_index = value[0];
        model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);
        auto type_id = column.GetTypeId();
        if (type_id == +model::TypeId::kUndefined) {
            throw std::invalid_argument("Column with index \"" + std::to_string(column_index)
                                        + "\" type undefined.");
        }
        if (type_id == +model::TypeId::kMixed) {
            throw std::invalid_argument("Column with index \"" + std::to_string(column_index)
                                        + "\" contains values of different types.");
        }

        if (metric_ == +Metric::euclidean) {
            if (!column.IsNumeric()) {
                throw std::invalid_argument("\"Euclidean\" metric is only available for numeric "
                                            "columns.");
            }
            return;
        }
        if (type_id == +model::TypeId::kString) return;
        throw std::invalid_argument("The chosen metric is available only for string columns.");
    }
    assert(value.size() > 0);
    if (metric_ == +Metric::euclidean) {
        for (auto column_index : value) {
            model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);
            auto type_id = column.GetTypeId();
            if (type_id == +model::TypeId::kUndefined) {
                throw std::invalid_argument("Column with index \"" + std::to_string(column_index)
                                            + "\" type undefined.");
            }
            if (type_id == +model::TypeId::kMixed) {
                throw std::invalid_argument("Column with index \"" + std::to_string(column_index)
                                            + "\" contains values of different types.");
            }

            if (!column.IsNumeric()) {
                throw std::invalid_argument("\"Euclidean\" metric is only available for numeric "
                                            "columns, column with index "
                                            + std::to_string(column_index) + " is not numeric");
            }
        }
        return;
    }
    throw std::invalid_argument("Multidimensional RHS is not available for the chosen metric");
}

void MetricVerifier::RegisterOptions() {
    auto check_lhs = [this](auto value) { ValidateIndices(value); };
    auto check_rhs = [this](auto value) { ValidateRhs(value); };
    auto need_algo_and_q = [this](...) {
        return metric_ == +Metric::cosine;
    };
    auto need_algo_only = [this](decltype(MetricVerifier::rhs_indices_) const& value) {
        assert(metric_ == +Metric::levenshtein || metric_ == +Metric::euclidean);
        return metric_ == +Metric::levenshtein || value.size() != 1;
    };
    auto algo_check = [this](MetricAlgo value) {
        assert(!(metric_ == +Metric::euclidean && rhs_indices_.size() == 1));
        if (value == +MetricAlgo::calipers) {
            if (!(metric_ == +Metric::euclidean && rhs_indices_.size() == 2))
                throw std::invalid_argument("\"calipers\" algorithm is only available for "
                                            "2-dimensional RHS and \"euclidean\" metric.");
        }
    };

    RegisterOption(config::EqualNullsOpt.GetOption(&is_null_equal_null_));
    RegisterOption(DistToNullInfinityOpt.GetOption(&dist_to_null_infinity_));
    RegisterOption(ParameterOpt.GetOption(&parameter_));
    RegisterOption(LhsIndicesOpt.GetOption(&lhs_indices_).SetInstanceCheck(check_lhs));
    RegisterOption(MetricOpt.GetOption(&metric_).SetConditionalOpts(
            GetOptAvailFunc(), {{{}, config::GetOptionNames(RhsIndicesOpt)}}));
    RegisterOption(
            RhsIndicesOpt.GetOption(&rhs_indices_).SetInstanceCheck(check_rhs)
                    .SetConditionalOpts(
                            GetOptAvailFunc(),
                            {{need_algo_and_q, config::GetOptionNames(AlgoOpt, QGramLengthOpt)},
                             {need_algo_only, config::GetOptionNames(AlgoOpt)}}));

    RegisterOption(AlgoOpt.GetOption(&algo_).SetInstanceCheck(algo_check));
    RegisterOption(QGramLengthOpt.GetOption(&q_));
}

void MetricVerifier::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable(config::GetOptionNames(DistToNullInfinityOpt, ParameterOpt, LhsIndicesOpt,
                                                MetricOpt));
}

void MetricVerifier::FitInternal(model::IDatasetStream& data_stream) {
    relation_ = ColumnLayoutRelationData::CreateFrom(data_stream, is_null_equal_null_);
    data_stream.Reset();
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: metric FD verifying is meaningless.");
    }
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(data_stream,
                                                                       is_null_equal_null_);
}

unsigned long long MetricVerifier::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();

    VerifyMetricFD();

    if (metric_fd_holds_) {
        LOG(DEBUG) << "Metric fd holds.";
    } else {
        LOG(DEBUG) << "Metric fd does not hold.";
    }

    SortHighlightsByDistance();

    VisualizeHighlights();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

std::string MetricVerifier::GetStringValue(std::vector<unsigned> const& index_vec,
                                           ClusterIndex row_index) const {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(index_vec[0]);
    if (col.IsNull(row_index)) {
        return "NULL";
    }
    if (col.IsEmpty(row_index)) {
        return "EMPTY";
    }
    if (index_vec.size() == 1) {
        return col.GetType().ValueToString(col.GetData()[row_index]);
    }
    std::string value("(");
    for (size_t j = 0; j < index_vec.size(); ++j) {
        model::TypedColumnData const& coord_col = typed_relation_->GetColumnData(index_vec[j]);
        value += coord_col.GetType().ValueToString(coord_col.GetData()[row_index]);
        if (j == index_vec.size() - 1) {
            break;
        }
        value += ", ";
    }
    value.push_back(')');
    return value;
}

void MetricVerifier::VisualizeHighlights() const {
    if (highlights_.empty()) {
        return;
    }
    for (auto const& cluster_highlight : highlights_) {
        LOG(DEBUG) << "----------------------------------------- LHS value: "
                   << GetStringValue(lhs_indices_, cluster_highlight[0].data_index);
        for (auto const& highlight : cluster_highlight) {
            bool is_null =
                    typed_relation_->GetColumnData(rhs_indices_[0]).IsNull(highlight.data_index);
            bool is_empty =
                    typed_relation_->GetColumnData(rhs_indices_[0]).IsEmpty(highlight.data_index);
            std::string value = GetStringValue(rhs_indices_, highlight.data_index);
            std::string begin_desc;
            if (!is_empty) {
                begin_desc = std::string("[") + (highlight.max_distance <= parameter_ ? "✓" : "X") +
                             "] | max dist: " + std::to_string(highlight.max_distance) + "\t| ";
            }
            std::string end_desc;
            if (!is_null && !is_empty) {
                end_desc = "\t| furthest point index: " +
                           std::to_string(highlight.furthest_data_index) +
                           "\t| furthest point value: " +
                           GetStringValue(rhs_indices_, highlight.furthest_data_index);
            }
            LOG(DEBUG) << begin_desc << "index: " << highlight.data_index << "\t| value: " << value
                       << end_desc;
        }
    }
}

void MetricVerifier::VerifyMetricFD() {
    std::shared_ptr<util::PLI const> pli =
            relation_->GetColumnData(lhs_indices_[0]).GetPliOwnership();

    for (size_t i = 1; i < lhs_indices_.size(); ++i) {
        pli = pli->Intersect(relation_->GetColumnData(lhs_indices_[i]).GetPositionListIndex());
    }

    metric_fd_holds_ = true;
    auto cluster_func = GetClusterFunction();
    for (auto const& cluster : pli->GetIndex()) {
        if (!cluster_func(cluster)) {
            metric_fd_holds_ = false;
            if (algo_ == +MetricAlgo::approx) {
                break;
            }
        }
    }
}

MetricVerifier::ClusterFunction MetricVerifier::GetClusterFunctionForOneDimension() {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);

    if (metric_ == +Metric::euclidean) {
        assert(col.IsNumeric());
        return CalculateClusterFunction<IndexedOneDimensionalPoint>(
                [this](auto const& cluster) { return CalculateIndexedPoints(cluster); },
                [this](auto const& points) { return CompareNumericValues(points); },
                [this](auto const& points, std::vector<Highlight>&& cluster_highlights) {
                    return CalculateOneDimensionalHighlights(points, std::move(cluster_highlights));
                });
    }

    assert(col.GetTypeId() == +model::TypeId::kString);
    auto const& type = static_cast<model::StringType const&>(col.GetType());

    std::function<ClusterFunction(DistanceFunction<std::byte const*>)> verify_func;
    if (algo_ == +MetricAlgo::brute) {
        verify_func = [this](auto dist_func) {
            return CalculateClusterFunction<IndexedOneDimensionalPoint>(
                    [this](auto const& cluster) { return CalculateIndexedPoints(cluster); },
                    [this, dist_func](auto const& points) {
                        return this->BruteVerifyCluster(points, dist_func);
                    },
                    [this, dist_func](auto const& points,
                                      std::vector<Highlight>&& cluster_highlights) {
                        return this->BruteCalculateHighlights(points, std::move(cluster_highlights),
                                                              dist_func);
                    });
        };
    } else {
        verify_func = [this](auto const& dist_func) {
            return CalculateApproxClusterFunction<std::byte const*>(
                    [this](auto const& cluster) { return CalculatePoints(cluster); }, dist_func);
        };
    }

    if (metric_ == +Metric::levenshtein) {
        return verify_func(
                [&type](std::byte const* l, std::byte const* r) { return type.Dist(l, r); });
    }

    return [this, &type, verify_func](util::PLI::Cluster const& cluster) {
        std::unordered_map<std::string, util::QGramVector> q_gram_map;
        return verify_func(GetCosineDistFunction(type, q_gram_map))(cluster);
    };
}

MetricVerifier::ClusterFunction MetricVerifier::GetClusterFunctionForSeveralDimensions() {
    if (algo_ == +MetricAlgo::calipers) {
        return [this](util::PLI::Cluster const& cluster) {
            auto result = CalculateMultidimensionalPointsForCalipers(cluster);
            if (!CheckMFDFailIfHasNulls(result.has_nulls) &&
                CalipersCompareNumericValues(result.points)) {
                return true;
            }

            auto result_indexed = CalculateMultidimensionalIndexedPoints(cluster);
            BruteCalculateHighlights<std::vector<long double>>(
                    result_indexed.points, std::move(result_indexed.cluster_highlights),
                    util::EuclideanDistance);
            return false;
        };
    }
    if (algo_ == +MetricAlgo::brute) {
        return CalculateClusterFunction<IndexedVector>(
                [this](auto const& cluster) {
                    return CalculateMultidimensionalIndexedPoints(cluster);
                },
                [this](auto const& points) {
                    return BruteVerifyCluster<std::vector<long double>>(points,
                                                                        util::EuclideanDistance);
                },
                [this](auto const& points, std::vector<Highlight>&& cluster_highlights) {
                    return BruteCalculateHighlights<std::vector<long double>>(
                            points, std::move(cluster_highlights), util::EuclideanDistance);
                });
    }
    auto points_func = [this](auto const& cluster) {
        return CalculateMultidimensionalPointsForApprox(cluster);
    };
    return CalculateApproxClusterFunction<std::vector<long double>>(points_func,
                                                                    util::EuclideanDistance);
}

MetricVerifier::ClusterFunction MetricVerifier::GetClusterFunction() {
    if (rhs_indices_.size() == 1) {
        return GetClusterFunctionForOneDimension();
    }
    return GetClusterFunctionForSeveralDimensions();
}

MetricVerifier::DistanceFunction<std::byte const*> MetricVerifier::GetCosineDistFunction(
        model::StringType const& type,
        std::unordered_map<std::string, util::QGramVector>& q_gram_map) const {
    return [this, &type, &q_gram_map](std::byte const* a, std::byte const* b) -> long double {
        std::string str1 = type.ValueToString(a);
        std::string str2 = type.ValueToString(b);
        if (str1.length() < q_ || str2.length() < q_) {
            throw std::runtime_error(
                    "q-gram length should not exceed the minimum string length "
                    "in the dataset.");
        }
        util::QGramVector const& v1 = q_gram_map.try_emplace(str1, str1, q_).first->second;
        util::QGramVector const& v2 = q_gram_map.try_emplace(str2, str2, q_).first->second;
        return v1.CosineDistance(v2);
    };
}

template <typename T>
MetricVerifier::ClusterFunction MetricVerifier::CalculateClusterFunction(
        IndexedPointsFunction<T> points_func, CompareFunction<T> compare_func,
        HighlightFunction<T> highlight_func) const {
    return [this, points_func, compare_func, highlight_func](util::PLI::Cluster const& cluster) {
        auto result = points_func(cluster);
        if (!CheckMFDFailIfHasNulls(result.has_nulls) && compare_func(result.points)) {
            return true;
        }
        highlight_func(result.points, std::move(result.cluster_highlights));
        return false;
    };
}

template <typename T>
MetricVerifier::ClusterFunction MetricVerifier::CalculateApproxClusterFunction(
        PointsFunction<T> points_func, DistanceFunction<T> dist_func) const {
    return [points_func, dist_func, this](util::PLI::Cluster const& cluster) {
        auto result = points_func(cluster);
        return !CheckMFDFailIfHasNulls(result.has_nulls) &&
               ApproxVerifyCluster(result.points, dist_func);
    };
}

bool MetricVerifier::CompareNumericValues(
        std::vector<IndexedOneDimensionalPoint> const& points) const {
    if (points.size() < 2) {
        return true;
    }
    model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
    auto const& type = static_cast<model::INumericType const&>(col.GetType());

    std::byte const* max_value = points[0].point;
    std::byte const* min_value = points[0].point;
    for (size_t i = 1; i < points.size(); ++i) {
        if (type.Compare(points[i].point, max_value) == model::CompareResult::kGreater) {
            max_value = points[i].point;
        } else if (type.Compare(points[i].point, min_value) == model::CompareResult::kLess) {
            min_value = points[i].point;
        }
        if (type.Dist(max_value, min_value) > parameter_) {
            return false;
        }
    }
    return true;
}

void MetricVerifier::CalculateOneDimensionalHighlights(
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

long double MetricVerifier::GetCoordinate(bool& has_values, ClusterIndex row_index, bool& has_nulls,
                                          unsigned col_index, bool& has_empties) const {
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
T MetricVerifier::GetPoint(ClusterIndex row_index, bool& has_values, bool& has_nulls,
                           bool& has_empties, AssignmentFunction<T> const& assignment_func) const {
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

MetricVerifier::IndexedPointsCalculationResult<MetricVerifier::IndexedVector>
MetricVerifier::CalculateMultidimensionalIndexedPoints(util::PLI::Cluster const& cluster) const {
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
            cluster_highlights.emplace_back(i, i, GetDistToNull());
        } else if (has_empties) {
            cluster_highlights.emplace_back(i, i, 0.0);
        }
    }
    return {std::move(points), std::move(cluster_highlights), has_nulls_in_cluster};
}

MetricVerifier::IndexedPointsCalculationResult<MetricVerifier::IndexedOneDimensionalPoint>
MetricVerifier::CalculateIndexedPoints(util::PLI::Cluster const& cluster) const {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
    std::vector<std::byte const*> const& data = col.GetData();
    std::vector<IndexedPoint<std::byte const*>> points;
    std::vector<Highlight> cluster_highlights;
    bool has_nulls_in_cluster = false;
    for (auto i : cluster) {
        if (col.IsNull(i)) {
            has_nulls_in_cluster = true;
            cluster_highlights.emplace_back(i, i, GetDistToNull());
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
MetricVerifier::PointsCalculationResult<T> MetricVerifier::CalculateMultidimensionalPoints(
        util::PLI::Cluster const& cluster, AssignmentFunction<T> const& assignment_func) const {
    std::vector<T> points;
    bool has_nulls_in_cluster = false;
    for (auto i : cluster) {
        bool has_values = false;
        bool has_nulls = false;
        bool has_empties = false;
        T point = GetPoint(i, has_values, has_nulls, has_empties, assignment_func);
        if (CheckMFDFailIfHasNulls(has_nulls)) {
            return {{}, true};
        }
        has_nulls_in_cluster |= has_nulls;
        if (has_values) {
            points.push_back(std::move(point));
        }
    }
    return {std::move(points), has_nulls_in_cluster};
}

MetricVerifier::PointsCalculationResult<std::byte const*> MetricVerifier::CalculatePoints(
        util::PLI::Cluster const& cluster) const {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
    std::vector<std::byte const*> const& data = col.GetData();
    std::vector<std::byte const*> points;
    bool has_nulls_in_cluster = false;
    for (auto i : cluster) {
        if (col.IsNull(i)) {
            if (dist_to_null_infinity_) {
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

template <typename T>
bool MetricVerifier::ApproxVerifyCluster(std::vector<T> const& points,
                                         DistanceFunction<T> const& dist_func) const {
    if (points.size() < 2) {
        return true;
    }
    return std::all_of(std::next(points.cbegin()), points.cend(),
                       [this, &points, &dist_func](auto const& p) {
                           return dist_func(points[0], p) * 2 <= parameter_;
                       });
}

template <typename T>
bool MetricVerifier::BruteVerifyCluster(std::vector<IndexedPoint<T>> const& points,
                                        DistanceFunction<T> const& dist_func) const {
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        for (size_t j = i + 1; j < points.size(); ++j) {
            if (dist_func(points[i].point, points[j].point) > parameter_) {
                return false;
            }
        }
    }
    return true;
}

template <typename T>
void MetricVerifier::BruteCalculateHighlights(std::vector<IndexedPoint<T>> const& indexed_points,
                                              std::vector<Highlight>&& cluster_highlights,
                                              DistanceFunction<T> const& dist_func) {
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

bool MetricVerifier::CalipersCompareNumericValues(std::vector<util::Point>& points) const {
    auto pairs = util::GetAntipodalPairs(util::CalculateConvexHull(points));
    return std::all_of(pairs.cbegin(), pairs.cend(), [this](auto const& pair) {
        return util::Point::EuclideanDistance(pair.first, pair.second) <= parameter_;
    });
}

void MetricVerifier::AssignToVector(long double coord, std::vector<long double>& point,
                                    [[maybe_unused]] size_t j) {
    point.push_back(coord);
}

void MetricVerifier::AssignToIndexedVector(long double coord, IndexedVector& point,
                                           [[maybe_unused]] size_t j) {
    point.point.push_back(coord);
}

void MetricVerifier::AssignToPoint(long double coord, util::Point& point, size_t j) {
    if (j == 0) {
        point.x = coord;
    } else {
        point.y = coord;
    }
}

void MetricVerifier::UpdateHighlightMap(HighlightMap& highlight_map, ClusterIndex index,
                                        ClusterIndex furthest_index, long double dist) {
    auto it = highlight_map.try_emplace(index, index, furthest_index, dist).first;
    if (it->second.max_distance < dist) {
        it->second.max_distance = dist;
        it->second.furthest_data_index = furthest_index;
    }
}

}  // namespace algos
