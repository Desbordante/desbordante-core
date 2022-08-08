#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "algorithms/metric_verifier_enums.h"
#include "algorithms/options/equal_nulls_opt.h"
#include "algorithms/primitive.h"
#include "model/column_layout_relation_data.h"
#include "model/column_layout_typed_relation_data.h"
#include "util/qgram_vector.h"

namespace algos {

class MetricVerifier : public algos::Primitive {
public:
    struct Highlight {
        int data_index;
        long double max_distance;

        Highlight(int data_index, long double max_distance)
            : data_index(data_index), max_distance(max_distance) {}

        bool operator<(Highlight const& other) const {
            return max_distance > other.max_distance;
        }
    };

private:
    template <typename T>
    using DistanceFunction = std::function<long double(T, T)>;
    using CompareFunction = std::function<bool(util::PLI::Cluster const& cluster)>;
    template <typename T>
    using PointsHighlightsPair = std::pair<std::vector<T>, std::multiset<Highlight>>;

    template <typename T>
    struct IndexedPoint {
        T point;
        int index;
        IndexedPoint() {}
        IndexedPoint(T point, int index) : point(point), index(index) {}
        IndexedPoint(IndexedPoint&& p) : point(std::move(p.point)), index(p.index) {}
    };

    Metric metric_ = Metric::_values()[0];
    MetricAlgo algo_ = MetricAlgo::_values()[0];
    std::vector<unsigned int> lhs_indices_;
    std::vector<unsigned int> rhs_indices_;
    long double parameter_;
    unsigned int q_;
    bool dist_to_null_infinity_;
    config::EqNullsType is_null_equal_null_;

    bool metric_fd_holds_ = false;
    std::vector<std::multiset<Highlight>> highlights_;

    static const config::OptionType<decltype(dist_to_null_infinity_)> DistToNullInfinityOpt;
    static const config::OptionType<decltype(parameter_)> ParameterOpt;
    static const config::OptionType<decltype(lhs_indices_)> LhsIndicesOpt;
    static const config::OptionType<decltype(rhs_indices_)> RhsIndicesOpt;
    static const config::OptionType<decltype(metric_)> MetricOpt;
    static const config::OptionType<decltype(algo_)> AlgoOpt;
    static const config::OptionType<decltype(q_)> QGramLengthOpt;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::shared_ptr<ColumnLayoutRelationData> relation_; // temporarily parsing twice

    DistanceFunction<std::byte const*> GetCosineDistFunction(
        model::StringType const& type,
        std::unordered_map<std::string, util::QGramVector>& q_gram_map) const;

    PointsHighlightsPair<IndexedPoint<std::byte const*>>
    GetVectorOfPoints(util::PLI::Cluster const& cluster) const;
    template <typename T>
    PointsHighlightsPair<T> GetVectorOfMultidimensionalPoints(
        util::PLI::Cluster const& cluster,
        std::function<void(T&, long double, size_t)> const& assignment_func) const;

    bool CompareNumericValues(util::PLI::Cluster const& cluster);
    template <typename T>
    bool BruteVerifyCluster(std::vector<IndexedPoint<T>> const& indexed_points,
                            std::multiset<Highlight>& cluster_highlights,
                            DistanceFunction<T> const& dist_func);
    bool CalipersCompareNumericValues(util::PLI::Cluster const& cluster);
    CompareFunction GetCompareFunction();
    void VerifyMetricFD();
    void VisualizeHighlights() const;
    static_assert(std::is_same<decltype(MetricVerifier::lhs_indices_),
            decltype(MetricVerifier::rhs_indices_)>{}, "Types of indices must be the same");
    void ValidateIndices(decltype(MetricVerifier::lhs_indices_) const& indices) const;
    void ValidateRhs(decltype(MetricVerifier::rhs_indices_) const& indices);
    void RegisterOptions();

protected:
    void FitInternal(model::IDatasetStream &data_stream) override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    bool GetResult() const {
        return metric_fd_holds_;
    }

    std::vector<std::multiset<Highlight>> GetHighlights() const {
        return highlights_;
    }

    void SetParameter(long double parameter) {
        parameter_ = parameter;
    }

    MetricVerifier();
};

}  // namespace algos
