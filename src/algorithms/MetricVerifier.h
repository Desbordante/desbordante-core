#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ColumnLayoutRelationData.h"
#include "ColumnLayoutTypedRelationData.h"
#include "Primitive.h"
#include "QGramVector.h"

namespace algos {

BETTER_ENUM(Metric, char,
            euclidean = 0,  /* Standard metric for calculating the distance between numeric
                             * values */
            levenshtein,    /* Levenshtein distance between strings */
            cosine          /* Represent strings as q-gram vectors and calculate cosine distance
                             * between these vectors */
)

BETTER_ENUM(MetricAlgo, char,
            brute = 0,      /* Brute force algorithm. Calculates distance between all possible pairs
                             * of values and compares them with parameter */
            approx,         /* 2-approximation brute force algorithm, that skips some distance
                             * calculations, but may return false positive result */
            calipers        /* Rotating calipers algorithm for 2d euclidean metric. Computes a
                             * convex hull of the points and calculates distances between antipodal
                             * pairs of convex hull, resulting in a significant reduction in the
                             * number of comparisons */
)

class MetricVerifier : public algos::Primitive {
private:
    using DistanceFunction = std::function<long double(std::byte const*, std::byte const*)>;
    using CompareFunction = std::function<bool(util::PLI::Cluster const& cluster)>;

    Metric metric_;
    MetricAlgo algo_ = MetricAlgo::_values()[0];
    std::vector<unsigned int> lhs_indices_;
    std::vector<unsigned int> rhs_indices_;
    long double parameter_;
    unsigned int q_;
    bool dist_to_null_infinity_;

    bool metric_fd_holds_ = false;

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
    void ValidateParameters() const;

public:
    struct Config {
        std::filesystem::path data{};   /* Path to input file */
        char separator = ',';           /* Separator for csv */
        bool has_header = true;         /* Indicates if input file has header */
        bool is_null_equal_null = true; /* Is NULL value equals another NULL value */
        std::string metric;             /* Metric to verify metric FD */
        std::string algo;               /* MFD algorithm */
        long double parameter;          /* The max possible distance between 2 values in cluster
                                         * at which metric FD holds */
        std::vector<unsigned int> lhs_indices; /* Indices of LHS columns */
        std::vector<unsigned int> rhs_indices; /* Indices of RHS columns */
        bool dist_to_null_infinity;     /* Determines whether distance to NULL value is infinity
                                         * or zero */
        unsigned int q;                 /* Q-gram length for cosine metric */
    };

    unsigned long long Execute() override;

    bool GetResult() const {
        return metric_fd_holds_;
    }

    void SetParameter(long double parameter) {
        parameter_ = parameter;
    }

    explicit MetricVerifier(Config const& config);

    explicit MetricVerifier(Config const& config,
                            std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation,
                            std::shared_ptr<ColumnLayoutRelationData> relation);
};

}  // namespace algos
