#pragma once

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
#include "util/qgram_vector.h"

namespace algos {

class MetricVerifier : public algos::Primitive {
private:
    using DistanceFunction = std::function<long double(std::byte const*, std::byte const*)>;
    using CompareFunction = std::function<bool(util::PLI::Cluster const& cluster)>;

    Metric metric_ = Metric::_values()[0];
    MetricAlgo algo_ = MetricAlgo::_values()[0];
    std::vector<unsigned int> lhs_indices_;
    std::vector<unsigned int> rhs_indices_;
    long double parameter_;
    unsigned int q_;
    bool dist_to_null_infinity_;
    config::EqNullsType is_null_equal_null_;

    bool metric_fd_holds_ = false;

    static const config::OptionType<decltype(dist_to_null_infinity_)> DistToNullInfinityOpt;
    static const config::OptionType<decltype(parameter_)> ParameterOpt;
    static const config::OptionType<decltype(lhs_indices_)> LhsIndicesOpt;
    static const config::OptionType<decltype(rhs_indices_)> RhsIndicesOpt;
    static const config::OptionType<decltype(metric_)> MetricOpt;
    static const config::OptionType<decltype(algo_)> AlgoOpt;
    static const config::OptionType<decltype(q_)> QGramLengthOpt;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::shared_ptr<ColumnLayoutRelationData> relation_; // temporarily parsing twice

    bool CompareNumericValues(util::PLI::Cluster const& cluster) const;
    bool CompareStringValues(util::PLI::Cluster const& cluster,
                             DistanceFunction const& dist_func) const;
    DistanceFunction GetCosineDistFunction(
        model::StringType const& type,
        std::unordered_map<std::string, util::QGramVector>& q_gram_map) const;

    template <typename T>
    std::vector<T> GetVectorOfPoints(
        util::PLI::Cluster const& cluster,
        std::function<void(T&, long double, size_t)> const& assignment_func) const;

    bool CompareNumericMultiDimensionalValues(util::PLI::Cluster const& cluster) const;
    bool CalipersCompareNumericValues(util::PLI::Cluster const& cluster) const;
    CompareFunction GetCompareFunction() const;
    bool VerifyMetricFD() const;
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

    void SetParameter(long double parameter) {
        parameter_ = parameter;
    }

    MetricVerifier();
};

}  // namespace algos
