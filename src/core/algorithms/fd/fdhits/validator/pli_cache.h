#pragma once

#include <cstddef>
#include <vector>

#include "core/algorithms/fd/fdhits/pli_table.h"
#include "core/algorithms/fd/fdhits/treesearch/node_type.h"
#include "core/algorithms/fd/fdhits/validator/cluster_collectors.h"

namespace algos::fd::fdhits {

template <typename E, typename S>
class PLICache {
private:
    std::vector<Pli> intersections_;
    size_t cached_;
    PLITable const* table_;
    S const* sampling_;

    void PullUpIntersections(std::vector<NodeType> const& columns, std::vector<E> const& cand,
                             ClusterFilter* filter);

public:
    PLICache(PLITable const* table, S const* sampling)
        : cached_(0), table_(table), sampling_(sampling) {
        intersections_.resize(table->GetColumnCount() * 2);
    }

    Pli const& GetPLI(std::vector<NodeType> const& columns, ClusterFilter* filter,
                      std::vector<E> const& cand);

    void Truncate(size_t current) {
        if (current < cached_) cached_ = current;
    }
};

template <typename E, typename S>
Pli const& PLICache<E, S>::GetPLI(std::vector<NodeType> const& columns, ClusterFilter* filter,
                                  std::vector<E> const& cand) {
    if (columns.empty()) return table_->GetFullPli();

    if (columns.size() == 1) {
        if (auto* lhs = std::get_if<size_t>(&columns[0])) {
            return table_->GetColumnPli(*lhs);
        }
        return table_->GetFullPli();
    }

    PullUpIntersections(columns, cand, filter);
    return intersections_[columns.size() - 2];
}

template <typename E, typename S>
void PLICache<E, S>::PullUpIntersections(std::vector<NodeType> const& columns,
                                         std::vector<E> const& cand, ClusterFilter* filter) {
    for (size_t i = cached_; i < columns.size(); ++i) {
        cached_++;
        if (cached_ <= 1) continue;

        size_t const idx = cached_ - 2;

        SamplingStrategyFilter<E, S, ClusterFilter> current_filter(&cand[cached_], sampling_,
                                                                   filter);

        Pli const* current = nullptr;
        if (idx == 0) {
            if (auto* lhs = std::get_if<size_t>(&columns[0])) {
                current = &table_->GetColumnPli(*lhs);
            } else {
                current = &table_->GetFullPli();
            }
        } else {
            current = &intersections_[idx - 1];
        }

        intersections_[idx].clear();
        current_filter.SetRequestedLevel(cached_ == columns.size());

        if (auto* lhs = std::get_if<size_t>(&columns[i])) {
            table_->Intersect(*current, idx == 0, *lhs, current_filter, intersections_[idx]);
        } else {
            for (auto const& cluster : *current) {
                if (current_filter.Keep(cluster)) {
                    intersections_[idx].push_back(cluster);
                }
            }
        }

        current_filter.SetRequestedLevel(false);
    }
}

}  // namespace algos::fd::fdhits
