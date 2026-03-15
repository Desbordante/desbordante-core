#pragma once

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "core/algorithms/fd/fdhits/edgecollectors/edge_hash_collector.h"
#include "core/algorithms/fd/fdhits/pli_table.h"
#include "core/algorithms/fd/fdhits/treesearch/hypergraph.h"
#include "core/algorithms/fd/fdhits/treesearch/node_type.h"
#include "core/algorithms/fd/fdhits/validator/cluster_collectors.h"
#include "core/algorithms/fd/fdhits/validator/pli_cache.h"
#include "core/algorithms/fd/fdhits/validator/validator.h"

namespace algos::fd::fdhits {

template <typename E, typename Consumer, typename Sampling, typename Problem>
class PLIValidator : public Validator<E> {
private:
    Consumer* consumer_;
    Sampling const* sampling_;
    Problem const* problem_;
    PLICache<E, Sampling> cache_;

public:
    PLIValidator(PLITable const* table, Consumer* consumer, Problem const* problem,
                 Sampling const* sampling)
        : consumer_(consumer), sampling_(sampling), problem_(problem), cache_(table, sampling) {}

    std::pair<bool, std::optional<Hypergraph<E>>> Check(E const& s,
                                                        std::vector<NodeType> const& columns,
                                                        std::vector<E> const& cand) override {
        EdgeHashCollector<E> set;
        SmallClusterCollector<E, Sampling, EdgeHashCollector<E>> filter(sampling_, &set, 100, true);

        auto const& pli = cache_.GetPLI(columns, &filter, cand);

        for (auto const& cluster : pli) {
            if (auto pair = sampling_->GetViolatingPair(cluster, s)) {
                auto [p1, p2] = pair.value();
                problem_->GetEdge(p1, p2, &set);
                break;
            }
        }

        bool is_valid = std::all_of(set.GetEdgeSet().begin(), set.GetEdgeSet().end(),
                                    [&s](E const& e) { return s.Covers(e); });

        if (is_valid) {
            if (problem_->IsValidResult(s)) {
                consumer_->Consume(s);
            }
            return {false, set.ToGraph(s)};
        }

        sampling_->SamplePLI(pli, &set);
        return {true, set.ToGraph(s)};
    }

    void Push(std::vector<NodeType> const& columns) override {
        if (!columns.empty()) {
            cache_.Truncate(columns.size() - 1);
        }
    }
};

}  // namespace algos::fd::fdhits
