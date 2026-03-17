#include "core/algorithms/fd/fdhits/pli_table.h"

#include <algorithm>

#include "core/algorithms/fd/hycommon/util/pli_util.h"

namespace algos::fd::fdhits {

thread_local RefinementHelper PLITable::refinement_helper_;

PLITable::PLITable(hy::PLIsPtr plis, std::shared_ptr<hy::Columns> inverse, hy::RowsPtr records,
                   std::optional<std::string> name)
    : plis_(std::move(plis)),
      inverse_(std::move(inverse)),
      compressed_records_(std::move(records)),
      row_count_(inverse_->at(0).size()),
      name_(std::move(name)) {
    for (auto& col : *inverse_) {
        for (auto& val : col) {
            if (hy::PLIUtil::IsSingletonCluster(val)) {
                val = 0;
            } else {
                val += 1;
            }
        }
    }

    if (row_count_ > 0) {
        Cluster full_cluster;
        full_cluster.reserve(row_count_);
        for (RowIndex i = 0; i < row_count_; ++i) {
            full_cluster.push_back(i);
        }
        full_pli_.push_back(std::move(full_cluster));
    }
}

void PLITable::SortPlisBy(std::size_t target) {
    SortPliBy(full_pli_, (*inverse_)[target]);
}

void PLITable::SortPliBy(Pli& pli, std::vector<ClusterIndex> const& target) {
    refinement_helper_.Reserve(target.size());

    for (Cluster& cluster : pli) {
        if (cluster.empty()) continue;

        ClusterIndex first = target[cluster[0]];
        bool needs_refinement =
                first == 0 || std::any_of(cluster.begin(), cluster.end(),
                                          [&](RowIndex row) { return target[row] != first; });

        if (needs_refinement) {
            for (RowIndex row : cluster) {
                refinement_helper_.Add(target[row], row);
            }
            refinement_helper_.UpdateCluster(cluster);
        }
    }
}

void PLITable::Intersect(Pli const& pli, bool first, std::size_t column, ClusterFilter& filter,
                         Pli& intersection) const {
    refinement_helper_.Reserve(row_count_);

    for (Cluster const& cluster : pli) {
        if (first && !filter.Keep(cluster)) {
            continue;
        }

        for (RowIndex row : cluster) {
            ClusterIndex cluster_id = (*inverse_)[column][row];
            if (cluster_id != 0) {
                refinement_helper_.Add(cluster_id, row);
            }
        }
        refinement_helper_.AddRefinedCluster(intersection, filter);
    }
}

}  // namespace algos::fd::fdhits
