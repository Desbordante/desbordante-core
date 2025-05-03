#pragma once

#include <cstddef>        // for byte
#include <memory>         // for unique_ptr, shar...
#include <string>         // for string
#include <unordered_map>  // for unordered_map
#include <vector>         // for vector

#include <enum.h>  // for _iterable

#include "algorithms/algorithm.h"                    // for Algorithm
#include "algorithms/metric/aliases.h"               // for DistanceFunction
#include "algorithms/metric/enums.h"                 // for Metric, MetricAlgo
#include "algorithms/metric/highlight_calculator.h"  // for HighlightCalculator
#include "algorithms/metric/points_calculator.h"     // for PointsCalculator
#include "config/equal_nulls/type.h"                 // for EqNullsType
#include "config/indices/type.h"                     // for IndicesType
#include "config/tabular_data/input_table_type.h"    // for InputTable

class ColumnLayoutRelationData;

namespace algos {
namespace metric {
struct Highlight;
}
}  // namespace algos

namespace algos {
namespace metric {
template <typename T>
struct IndexedPoint;
}
}  // namespace algos

namespace model {
class ColumnLayoutTypedRelationData;
}

namespace model {
class StringType;
}

namespace util {
class QGramVector;
}

namespace util {
struct Point;
}

namespace algos::metric {

class MetricVerifier : public Algorithm {
private:
    config::InputTable input_table_;

    Metric metric_ = Metric::_values()[0];
    MetricAlgo algo_ = MetricAlgo::_values()[0];
    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;
    long double parameter_;
    unsigned int q_;
    bool dist_from_null_is_infinity_;
    config::EqNullsType is_null_equal_null_;

    bool metric_fd_holds_ = false;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::shared_ptr<ColumnLayoutRelationData> relation_;  // temporarily parsing twice
    std::unique_ptr<PointsCalculator> points_calculator_;
    std::unique_ptr<HighlightCalculator> highlight_calculator_;

    DistanceFunction<std::byte const*> GetCosineDistFunction(
            model::StringType const& type,
            std::unordered_map<std::string, util::QGramVector>& q_gram_map) const;

    bool CheckMFDFailIfHasNulls(bool has_nulls) const {
        return dist_from_null_is_infinity_ && has_nulls;
    }

    bool CompareNumericValues(std::vector<IndexedOneDimensionalPoint> const& points) const;

    template <typename T>
    bool ApproxVerifyCluster(std::vector<T> const& points,
                             DistanceFunction<T> const& dist_func) const;

    template <typename T>
    bool BruteVerifyCluster(std::vector<IndexedPoint<T>> const& points,
                            DistanceFunction<T> const& dist_func) const;

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
    std::string GetStringValue(config::IndicesType const& index_vec, ClusterIndex row_index) const;
    void VisualizeHighlights() const;
    void ValidateRhs(config::IndicesType const& rhs_indices);
    void RegisterOptions();

    void ResetState() final;

protected:
    void LoadDataInternal() final;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    bool GetResult() const {
        return metric_fd_holds_;
    }

    config::IndicesType const& GetLhsIndices() const {
        return lhs_indices_;
    }

    config::IndicesType const& GetRhsIndices() const {
        return rhs_indices_;
    }

    long double GetParameter() const {
        return parameter_;
    }

    std::vector<std::vector<Highlight>> const& GetHighlights() const {
        return highlight_calculator_->GetHighlights();
    }

    void SetParameter(long double parameter) {
        parameter_ = parameter;
    }

    void SortHighlightsByDistanceAscending() {
        highlight_calculator_->SortHighlightsByDistanceAscending();
    }

    void SortHighlightsByDistanceDescending() {
        highlight_calculator_->SortHighlightsByDistanceDescending();
    }

    void SortHighlightsByFurthestIndexAscending() {
        highlight_calculator_->SortHighlightsByFurthestIndexAscending();
    }

    void SortHighlightsByFurthestIndexDescending() {
        highlight_calculator_->SortHighlightsByFurthestIndexDescending();
    }

    void SortHighlightsByIndexAscending() {
        highlight_calculator_->SortHighlightsByIndexAscending();
    }

    void SortHighlightsByIndexDescending() {
        highlight_calculator_->SortHighlightsByIndexDescending();
    }

    MetricVerifier();
};

}  // namespace algos::metric
