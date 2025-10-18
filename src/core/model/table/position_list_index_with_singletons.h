//
// Created by Emelyanov Maksim
//

#pragma once
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

#include "model/table/column.h"
#include "position_list_index.h"

class ColumnLayoutRelationData;

namespace model {

class PLIWithSingletons : public PositionListIndex {
private:
    std::deque<Cluster> singletons_;

public:
    PLIWithSingletons(std::deque<Cluster> index, std::deque<Cluster> singletons,
                      Cluster null_cluster, unsigned int size, double entropy,
                      unsigned long long nep, unsigned int relation_size,
                      unsigned int original_relation_size, double inverted_entropy = 0,
                      double gini_impurity = 0);

    PLIWithSingletons(PositionListIndex& positional_list_index)
        : PositionListIndex(std::move(positional_list_index)) {};

    static std::unique_ptr<PLIWithSingletons> CreateFor(std::vector<int>& data,
                                                        bool is_null_eq_null);

    /* Returns all clusters, including singletons */
    std::deque<Cluster> GetAllClusters() const {
        auto all_clusters(index_);
        all_clusters.insert(all_clusters.end(), singletons_.begin(), singletons_.end());
        return all_clusters;
    }

    std::unique_ptr<PLIWithSingletons> ProbeAll(Vertical const& probing_columns,
                                                ColumnLayoutRelationData& relation_data);

    std::unique_ptr<PLIWithSingletons> Probe(
            std::shared_ptr<std::vector<int> const> probing_table) const;

    std::unique_ptr<PLIWithSingletons> Intersect(PLIWithSingletons const* that) const;
};

using PLIWS = PLIWithSingletons;

}  // namespace model
