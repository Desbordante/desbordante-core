//
// Created by Ilya Vologin
// https://github.com/cupertank
//

#pragma once

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/pyrocommon/model/confidence_interval.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/util/custom_random.h"

namespace model {

// abstract base class for Agree Set Sample implementations (trie <- not used, list)
class AgreeSetSample {
public:
    virtual unsigned long long GetNumAgreeSupersets(
            boost::dynamic_bitset<> const& agreement) const = 0;
    virtual unsigned long long GetNumAgreeSupersets(
            boost::dynamic_bitset<> const& agreement,
            boost::dynamic_bitset<> const& disagreement) const = 0;
    virtual std::unique_ptr<std::vector<unsigned long long>> GetNumAgreeSupersetsExt(
            boost::dynamic_bitset<> const& agreement,
            boost::dynamic_bitset<> const& disagreement) const;

    double EstimateAgreements(boost::dynamic_bitset<> const& agreement) const;
    ConfidenceInterval EstimateAgreements(boost::dynamic_bitset<> const& agreement,
                                          double confidence) const;
    ConfidenceInterval EstimateMixed(boost::dynamic_bitset<> const& agreement,
                                     boost::dynamic_bitset<> const& disagreement,
                                     double confidence) const;

    double GetSamplingRatio() const {
        return sample_size_ / static_cast<double>(population_size_);
    }

    bool IsExact() const {
        return population_size_ == sample_size_;
    }

    virtual ~AgreeSetSample() = default;

protected:
    ::ColumnLayoutRelationData const* relation_data_;
    boost::dynamic_bitset<> focus_;
    unsigned int sample_size_;
    unsigned long long population_size_;
    AgreeSetSample(ColumnLayoutRelationData const* relation_data, boost::dynamic_bitset<> focus,
                   unsigned int sample_size, unsigned long long population_size);

    template <typename T>
    static std::unique_ptr<T> CreateFocusedFor(ColumnLayoutRelationData const* relation,
                                               boost::dynamic_bitset<> const& restriction_vertical,
                                               PositionListIndex const* restriction_pli,
                                               unsigned int sample_size, CustomRandom& random);

private:
    static constexpr double kStdDevSmoothing = 1;

    double RatioToRelationRatio(double ratio) const {
        return ratio * population_size_ / relation_data_->GetNumTuplePairs();
    }

    double ObservationsToRelationRatio(double num_observations) const {
        return RatioToRelationRatio(num_observations / sample_size_);
    }

    static double CalculateNonNegativeFraction(double a, double b);

    ConfidenceInterval EstimateGivenNumHits(unsigned long long num_hits, double confidence) const;
    // Inverse cumulative distribution function (aka the probit function)
    double ProbitFunction(double quantile) const;
};

}  // namespace model

// include template implementation
#include "core/algorithms/fd/pyrocommon/model/agree_set_sample_impl.h"
