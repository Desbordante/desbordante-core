#pragma once

#include <cstdint>

#include "core/algorithms/fd/fdhits/validator/cluster_filter.h"

namespace algos::fd::fdhits {

template <typename E, typename S, typename F>
class SamplingStrategyFilter : public ClusterFilter {
private:
    E const* candidate_;
    S const* sampling_;
    F* filter_;

public:
    SamplingStrategyFilter(E const* candidate, S const* sampling, F* filter)
        : candidate_(candidate), sampling_(sampling), filter_(filter) {}

    bool Keep(Cluster const& cluster) const override {
        return sampling_->GetViolatingPair(cluster, *candidate_).has_value() &&
               filter_->Keep(cluster);
    }

    void SetRequestedLevel(bool active) override {
        filter_->SetRequestedLevel(active);
    }
};

template <typename E, typename S, typename C>
class SmallClusterCollector : public ClusterFilter {
private:
    S const* sampling_;
    C* consumer_;
    uint64_t limit_;
    bool requested_level_;
    bool require_requested_level_;

public:
    SmallClusterCollector(S const* sampling, C* consumer, uint64_t limit,
                          bool require_requested_level)
        : sampling_(sampling),
          consumer_(consumer),
          limit_(limit),
          requested_level_(false),
          require_requested_level_(require_requested_level) {}

    bool Keep(Cluster const& cluster) const override {
        if ((!require_requested_level_ || requested_level_) && S::GetPairCount(cluster) < limit_) {
            sampling_->GetCrossProduct(cluster, consumer_);
            return false;
        }
        return cluster.size() > 1;
    }

    void SetRequestedLevel(bool active) override {
        requested_level_ = active;
    }
};

}  // namespace algos::fd::fdhits
