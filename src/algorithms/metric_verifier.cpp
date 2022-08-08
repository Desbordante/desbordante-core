#include "algorithms/metric_verifier.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>

#include <easylogging++.h>

#include "util/convex_hull.h"

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
        LOG(INFO) << "Metric fd holds.";
    } else {
        LOG(INFO) << "Metric fd does not hold.";
    }

    VisualizeHighlights();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void MetricVerifier::VisualizeHighlights() const {
    if (highlights_.empty()) return;
    LOG(INFO) << "-----------------------------------------";
    for (auto const& cluster_highlight : highlights_) {
        for (auto const& highlight : cluster_highlight) {
            std::string value;
            model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
            if (col.IsNullOrEmpty(highlight.data_index)) {
                value = "NULL";
            } else if (rhs_indices_.size() == 1) {
                value = col.GetType().ValueToString(col.GetData()[highlight.data_index]);
            } else {
                value.push_back('(');
                for (size_t j = 0; j < rhs_indices_.size(); ++j) {
                    model::TypedColumnData const& coord_col =
                        typed_relation_->GetColumnData(rhs_indices_[j]);
                    value += coord_col.GetType().ValueToString(
                        coord_col.GetData()[highlight.data_index]);
                    if (j == rhs_indices_.size() - 1) break;
                    value += ", ";
                }
                value.push_back(')');
            }
            LOG(INFO) << "[" << (highlight.max_distance <= parameter_ ? "✓" : "X")
                      << "] | max dist: " << highlight.max_distance
                      << "\t| i: " << highlight.data_index
                      << "\t| value: " << value;
        }
        LOG(INFO) << "-----------------------------------------";
    }
}

void MetricVerifier::VerifyMetricFD() {
    std::shared_ptr<util::PLI const>
        pli = relation_->GetColumnData(lhs_indices_[0]).GetPliOwnership();

    for (size_t i = 1; i < lhs_indices_.size(); ++i) {
        pli = pli->Intersect(relation_->GetColumnData(lhs_indices_[i]).GetPositionListIndex());
    }

    metric_fd_holds_ = true;
    auto compare_func = GetCompareFunction();
    for (auto const& cluster : pli->GetIndex()) {
        if (!compare_func(cluster)) {
            metric_fd_holds_ = false;
        }
    }
}

MetricVerifier::CompareFunction MetricVerifier::GetCompareFunction() {
    if (rhs_indices_.size() == 1) {
        model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
        switch (metric_) {
        case Metric::euclidean:
            assert(col.IsNumeric());
            return [this](util::PLI::Cluster const& cluster) {
                return CompareNumericValues(cluster);
            };
        case Metric::levenshtein:
            assert(col.GetTypeId() == +model::TypeId::kString);
            return [this, &col](util::PLI::Cluster const& cluster) {
                auto const& type = dynamic_cast<model::StringType const&>(col.GetType());
                auto [points, cluster_highlights] = GetVectorOfPoints(cluster);
                return BruteVerifyCluster<std::byte const*>(
                    points, cluster_highlights, [&type](std::byte const* a, std::byte const* b) {
                        return type.Dist(a, b);
                    });
            };
        case Metric::cosine:
            assert(col.GetTypeId() == +model::TypeId::kString);
            return [this, &col](util::PLI::Cluster const& cluster) {
                auto const& type = dynamic_cast<model::StringType const&>(col.GetType());
                std::unordered_map<std::string, util::QGramVector> q_gram_map;
                auto [points, cluster_highlights] = GetVectorOfPoints(cluster);
                return BruteVerifyCluster(points, cluster_highlights,
                                          GetCosineDistFunction(type, q_gram_map));
            };
        }
    }
    if (algo_ == +MetricAlgo::calipers) {
        return [this](util::PLI::Cluster const& cluster) {
            return CalipersCompareNumericValues(cluster);
        };
    }
    return [this](util::PLI::Cluster const& cluster) {
        auto [points, cluster_highlights] =
            GetVectorOfMultidimensionalPoints<IndexedPoint<std::vector<long double>>>(
                cluster, [](auto& indexed_point, long double coord, [[maybe_unused]] size_t j) {
                    indexed_point.point.push_back(coord);
                });
        return BruteVerifyCluster<std::vector<long double>>(points, cluster_highlights,
                                                            util::EuclideanDistance);
    };
}

MetricVerifier::DistanceFunction<std::byte const*> MetricVerifier::GetCosineDistFunction(
    model::StringType const& type,
    std::unordered_map<std::string, util::QGramVector>& q_gram_map) const {
    return [this, &type, &q_gram_map](std::byte const* a, std::byte const* b) -> long double {
        std::string str1 = type.ValueToString(a);
        std::string str2 = type.ValueToString(b);
        if (str1.length() < q_ || str2.length() < q_) {
            throw std::runtime_error("q-gram length should not exceed the minimum string length "
                                     "in the dataset.");
        }
        util::QGramVector& v1 = q_gram_map.try_emplace(str1, str1, q_).first->second;
        util::QGramVector& v2 = q_gram_map.try_emplace(str2, str2, q_).first->second;
        return v1.CosineDistance(v2);
    };
}

bool MetricVerifier::CompareNumericValues(util::PLI::Cluster const& cluster) {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
    auto const& type = dynamic_cast<model::INumericType const&>(col.GetType());
    auto [points, cluster_highlights] = GetVectorOfPoints(cluster);
    bool mfd_failed = dist_to_null_infinity_ && !cluster_highlights.empty();
    if (!mfd_failed && points.size() <= 1) {
        return true;
    }
    auto [min_value, max_value] =
        std::minmax_element(points.begin(), points.end(), [&type](auto const& a, auto const& b) {
            return type.Compare(a.point, b.point) == model::CompareResult::kLess;
        });
    if (!mfd_failed && type.Dist(min_value->point, max_value->point) <= parameter_) {
        return true;
    }
    for (auto const& indexed_point : points) {
        cluster_highlights.emplace(indexed_point.index,
                                   std::max(type.Dist(indexed_point.point, max_value->point),
                                            type.Dist(indexed_point.point, min_value->point)));
    }
    highlights_.push_back(std::move(cluster_highlights));
    return false;
}

static void UpdateDistanceMap(std::unordered_map<int, long double>& distance_map, int index,
                              long double dist) {
    auto it = distance_map.find(index);
    if (it == distance_map.end()) {
        distance_map.emplace(index, dist);
        return;
    }
    it->second = std::max(it->second, dist);
}

template <typename T>
MetricVerifier::PointsHighlightsPair<T> MetricVerifier::GetVectorOfMultidimensionalPoints(
    util::PLI::Cluster const& cluster,
    std::function<void(T&, long double, size_t)> const& assignment_func) const {
    std::vector<T> points;
    std::multiset<Highlight> null_highlights;
    for (int i : cluster) {
        bool has_values = false;
        bool has_nulls = false;
        T point;
        point.index = i;
        for (size_t j = 0; j < rhs_indices_.size(); ++j) {
            model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[j]);
            if (col.IsNullOrEmpty(i)) {
                if (has_values)
                    throw std::runtime_error("Some of the value coordinates are nulls.");
                if (j == rhs_indices_.size() - 1) {
                    null_highlights.emplace(i, dist_to_null_infinity_
                                               ? std::numeric_limits<long double>::infinity()
                                               : 0.0);
                }
                has_nulls = true;
                continue;
            }
            if (has_nulls) throw std::runtime_error("Some of the value coordinates are nulls.");
            long double coord =
                col.GetType().GetTypeId() == +model::TypeId::kInt
                ? (long double)model::Type::GetValue<long long>(col.GetData()[i])
                : model::Type::GetValue<long double>(col.GetData()[i]);
            assignment_func(point, coord, j);
            has_values = true;
        }
        if (has_values) points.push_back(std::move(point));
    }
    return {std::move(points), std::move(null_highlights)};
}

MetricVerifier::PointsHighlightsPair<MetricVerifier::IndexedPoint<std::byte const*>>
MetricVerifier::GetVectorOfPoints(
    util::PLI::Cluster const& cluster) const {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
    std::vector<std::byte const*> const& data = col.GetData();
    std::vector<IndexedPoint<std::byte const*>> points;
    std::multiset<Highlight> null_highlights;
    for (int i : cluster) {
        if (col.IsNullOrEmpty(i)) {
            null_highlights.emplace(i, dist_to_null_infinity_
                                       ? std::numeric_limits<long double>::infinity() : 0.0);
            continue;
        }
        points.emplace_back(data[i], i);
    }
    return {std::move(points), std::move(null_highlights)};
}

template <typename T>
bool MetricVerifier::BruteVerifyCluster(std::vector<IndexedPoint<T>> const& indexed_points,
                                        std::multiset<Highlight>& cluster_highlights,
                                        DistanceFunction<T> const& dist_func) {
    std::unordered_map<int, long double> distance_map;
    bool mfd_failed = dist_to_null_infinity_ && !cluster_highlights.empty();

    for (size_t i = 0; i + 1 < indexed_points.size(); ++i) {
        long double max_dist = 0;
        for (size_t j = i + 1; j < indexed_points.size(); ++j) {
            long double dist = dist_func(indexed_points[i].point, indexed_points[j].point);
            max_dist = std::max(max_dist, dist);
            UpdateDistanceMap(distance_map, indexed_points[j].index, dist);
            if (max_dist > parameter_) mfd_failed = true;
            else if (!mfd_failed && algo_ == +MetricAlgo::approx && max_dist * 2 < parameter_)
                return true;
        }
        UpdateDistanceMap(distance_map, indexed_points[i].index, max_dist);
    }
    if (mfd_failed) {
        for (auto const& pair : distance_map) {
            cluster_highlights.emplace(pair.first, pair.second);
        }
        if (indexed_points.size() == 1) {
            cluster_highlights.emplace(indexed_points[0].index, 0);
        }
        highlights_.push_back(std::move(cluster_highlights));
    }
    return !mfd_failed;
}

bool MetricVerifier::CalipersCompareNumericValues(const util::PLI::Cluster& cluster) {
    auto [points, cluster_highlights] = GetVectorOfMultidimensionalPoints<util::Point>(
        cluster, [](auto& point, long double coord, size_t j) {
            if (j == 0) point.x = coord;
            else point.y = coord;
        });
    bool mfd_failed = dist_to_null_infinity_ && !cluster_highlights.empty();
    auto pairs = util::GetAntipodalPairs(util::ConvexHull(points));
    std::unordered_map<int, long double> distance_map;

    for (auto const& pair : pairs) {
        long double dist = util::Point::EuclideanDistance(pair.first, pair.second);
        UpdateDistanceMap(distance_map, pair.first.index, dist);
        UpdateDistanceMap(distance_map, pair.second.index, dist);
        if (dist > parameter_) mfd_failed = true;
    }
    if (mfd_failed) {
        for (auto const& pair : distance_map) {
            cluster_highlights.emplace(pair.first, pair.second);
        }
        if (points.size() == 1) {
            cluster_highlights.emplace(points[0].index, 0);
        }
        highlights_.push_back(std::move(cluster_highlights));
    }
    return !mfd_failed;
}

}  // namespace algos
