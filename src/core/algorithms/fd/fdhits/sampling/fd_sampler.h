#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/algorithms/fd/fdhits/pli_table.h"
#include "core/algorithms/fd/fdhits/types.h"

namespace algos::fd::fdhits {

template <typename P>
class Sampler {
private:
    P const* problem_;
    PLITable const* table_;
    double sampling_factor_;
    mutable std::mt19937 rng_;

public:
    using EdgeType = typename P::EdgeType;

    Sampler(P const* problem, PLITable const* table, double sampling_factor)
        : problem_(problem),
          table_(table),
          sampling_factor_(sampling_factor),
          rng_(std::random_device{}()) {}

    template <typename C>
    void SamplePLI(Pli const& pli, C* consumer) const {
        if (pli.empty()) return;

        std::vector<uint64_t> weights;
        uint64_t total_pairs = 0;
        for (auto const& cluster : pli) {
            uint64_t pairs = GetPairCount(cluster);
            total_pairs += pairs;
            weights.push_back(pairs);
        }

        if (total_pairs == 0) return;

        size_t to_view =
                std::max(size_t{1}, static_cast<size_t>(std::pow(total_pairs, sampling_factor_)));

        std::discrete_distribution<size_t> dist(weights.begin(), weights.end());

        struct SampleKey {
            size_t cluster_idx;
            RowIndex r1, r2;

            bool operator==(SampleKey const& other) const {
                return cluster_idx == other.cluster_idx && r1 == other.r1 && r2 == other.r2;
            }
        };

        struct SampleKeyHash {
            size_t operator()(SampleKey const& k) const {
                return k.cluster_idx ^ (k.r1 << 16) ^ (k.r2 << 8);
            }
        };

        std::unordered_set<SampleKey, SampleKeyHash> samples;
        samples.reserve(to_view);

        for (size_t i = 0; i < to_view; ++i) {
            size_t cluster_idx = dist(rng_);
            auto const& cluster = pli[cluster_idx];
            if (cluster.size() < 2) continue;

            std::uniform_int_distribution<RowIndex> row_dist(0, cluster.size() - 1);
            RowIndex r1 = row_dist(rng_);
            RowIndex offset = std::uniform_int_distribution<RowIndex>(1, cluster.size() - 1)(rng_);
            RowIndex r2 = (r1 + offset) % cluster.size();

            samples.insert({cluster_idx, r1, r2});
        }

        for (auto const& sample : samples) {
            problem_->GetEdge(pli[sample.cluster_idx][sample.r1],
                              pli[sample.cluster_idx][sample.r2], consumer);
        }
    }

    std::optional<std::pair<RowIndex, RowIndex>> GetViolatingPair(Cluster const& cluster,
                                                                  EdgeType const& candidate) const {
        if (cluster.size() < 2) return std::nullopt;

        RowIndex r1 = cluster[0];
        for (size_t j = 1; j < cluster.size(); ++j) {
            if (problem_->IsViolatingPair(r1, cluster[j], candidate)) {
                return std::make_pair(r1, cluster[j]);
            }
        }
        return std::nullopt;
    }

    static uint64_t GetPairCount(Cluster const& cluster) {
        uint64_t size = cluster.size();
        return size * (size - 1) / 2;
    }

    template <typename C>
    void GetCrossProduct(Cluster const& cluster, C* consumer) const {
        for (size_t i = 0; i < cluster.size(); ++i) {
            for (size_t j = i + 1; j < cluster.size(); ++j) {
                problem_->GetEdge(cluster[i], cluster[j], consumer);
            }
        }
    }
};

}  // namespace algos::fd::fdhits
