#pragma once

#include <vector>

#include "core/algorithms/fd/fdhits/types.h"

namespace algos::fd::fdhits {

class RefinementHelper {
private:
    Pli refinement_table_;
    std::vector<ClusterIndex> active_;

public:
    RefinementHelper() = default;

    void Reserve(std::size_t size) {
        if (refinement_table_.size() < size) {
            refinement_table_.resize(size);
            active_.reserve(size);
        }
    }

    void Add(ClusterIndex cluster_id, RowIndex row) {
        Cluster& refined_cluster = refinement_table_[cluster_id];
        if (refined_cluster.empty()) {
            active_.push_back(cluster_id);
        }
        refined_cluster.push_back(row);
    }

    void AddRefinedCluster(Pli& target, ClusterFilter const& filter) {
        for (ClusterIndex cluster_id : active_) {
            Cluster& new_cluster = refinement_table_[cluster_id];
            if (filter.Keep(new_cluster)) {
                target.push_back(std::move(new_cluster));
                refinement_table_[cluster_id] = Cluster{};
            } else {
                new_cluster.clear();
            }
        }
        active_.clear();
    }

    void UpdateCluster(Cluster& target) {
        std::size_t i = 0;
        for (ClusterIndex cluster_id : active_) {
            Cluster& rows = refinement_table_[cluster_id];
            for (RowIndex row : rows) {
                target[i++] = row;
            }
            rows.clear();
        }
        active_.clear();
    }
};

}  // namespace algos::fd::fdhits
