//
// Created by Emelyanov Maksim
//

#include "position_list_index_with_singletons.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <map>
#include <memory>
#include <utility>

#include <boost/dynamic_bitset.hpp>

#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/vertical.h"
#include "core/util/logger.h"

namespace model {
PLIWithSingletons::PLIWithSingletons(std::deque<std::vector<int>> index,
                                     std::deque<std::vector<int>> singletons, unsigned int size,
                                     double entropy, unsigned long long nep,
                                     unsigned int relation_size, double inverted_entropy,
                                     double gini_impurity)
    : PositionListIndex(index, size, entropy, nep, relation_size, inverted_entropy, gini_impurity),
      singletons_(std::move(singletons)) {}

PLIWithSingletons::PLIWithSingletons(std::unique_ptr<PositionListIndex> positional_list_index)
    : PositionListIndex(*positional_list_index.get()) {
    Cluster sngt;

    std::shared_ptr<model::PositionListIndex::Cluster const> probing_table =
            CalculateAndGetProbingTable();

    for (size_t position = 0; position < probing_table->size(); position++) {
        if ((*probing_table)[position] == kSingletonValueId) sngt.push_back(position);
    }

    singletons_.push_back(std::move(sngt));
}

std::unique_ptr<PLIWithSingletons> PLIWithSingletons::CreateFor(std::vector<int>& data) {
    std::unordered_map<int, std::vector<int>> index;
    for (unsigned long position = 0; position < data.size(); ++position) {
        int value_id = data[position];
        index[value_id].push_back(position);
    }

    double key_gap = 0.0;
    double inv_ent = 0;
    double gini_gap = 0;
    unsigned long long nep = 0;
    unsigned int size = 0;
    std::deque<std::vector<int>> clusters;
    std::deque<std::vector<int>> singletons;

    for (auto& iter : index) {
        if (iter.second.size() == 1) {
            singletons.emplace_back(std::move(iter.second));
            gini_gap += std::pow(1 / static_cast<double>(data.size()), 2);
            continue;
        }
        key_gap += iter.second.size() * log(iter.second.size());
        nep += CalculateNep(iter.second.size());
        size += iter.second.size();
        inv_ent += -(1 - iter.second.size() / static_cast<double>(data.size())) *
                   std::log(1 - (iter.second.size() / static_cast<double>(data.size())));
        gini_gap += std::pow(iter.second.size() / static_cast<double>(data.size()), 2);

        clusters.emplace_back(std::move(iter.second));
    }
    double entropy = log(data.size()) - key_gap / data.size();

    double gini_impurity = 1 - gini_gap;
    if (gini_impurity == 0) {
        inv_ent = 0;
    }

    SortClusters(singletons);
    SortClusters(clusters);
    return std::make_unique<PLIWithSingletons>(std::move(clusters), std::move(singletons), size,
                                               entropy, nep, data.size(), inv_ent, gini_impurity);
}

std::unique_ptr<PLIWithSingletons> PLIWithSingletons::Probe(
        std::shared_ptr<std::vector<int> const> probing_table) const {
    if (this->relation_size_ != probing_table->size())
        throw std::invalid_argument("received different number of rows");
    std::deque<std::vector<int>> new_index;
    std::deque<std::vector<int>> singletons(singletons_);
    unsigned int new_size = 0;
    double new_key_gap = 0.0;
    unsigned long long new_nep = 0;

    std::unordered_map<int, std::vector<int>> partial_index;

    for (auto& positions : index_) {
        for (int position : positions) {
            if (probing_table == nullptr) LOG_DEBUG("NULLPTR");
            if (position < 0 || static_cast<size_t>(position) >= probing_table->size()) {
                LOG_DEBUG("position: {} size: {}", position, probing_table->size());
                for (size_t i = 0; i < positions.size(); ++i) {
                    LOG_DEBUG("Position {}", positions[i]);
                }
            }
            int probing_table_value_id = (*probing_table)[position];
            if (probing_table_value_id == kSingletonValueId) {
                partial_index[kSingletonValueId].push_back(position);
                continue;
            }
            partial_index[probing_table_value_id].push_back(position);
        }

        for (auto& iter : partial_index) {
            auto& cluster = iter.second;
            if (cluster.size() <= 1 || iter.first == kSingletonValueId) {
                singletons.push_back(std::move(cluster));
                continue;
            }

            new_size += cluster.size();
            new_key_gap += cluster.size() * log(cluster.size());
            new_nep += CalculateNep(cluster.size());

            new_index.push_back(std::move(cluster));
        }
        partial_index.clear();
    }

    double new_entropy = log(relation_size_) - new_key_gap / relation_size_;
    SortClusters(singletons);
    SortClusters(new_index);

    return std::make_unique<PLIWithSingletons>(std::move(new_index), std::move(singletons),
                                               new_size, new_entropy, new_nep, relation_size_,
                                               relation_size_);
}

std::unique_ptr<PLIWithSingletons> PLIWithSingletons::ProbeAll(
        Vertical const& probing_columns, ColumnLayoutRelationData& relation_data) {
    if (this->relation_size_ != relation_data.GetNumRows())
        throw std::invalid_argument("received different number of rows");
    std::deque<std::vector<int>> new_index;
    std::deque<std::vector<int>> singletons(singletons_);
    unsigned int new_size = 0;
    double new_key_gap = 0.0;
    unsigned long long new_nep = 0;

    std::map<std::vector<int>, std::vector<int>> partial_index;
    std::vector<int> probe;

    for (auto& cluster : this->index_) {
        for (int position : cluster) {
            if (!TakeProbe(position, relation_data, probing_columns, probe)) {
                partial_index[{kSingletonValueId}].push_back(position);
                probe.clear();
                continue;
            }

            partial_index[probe].push_back(position);
            probe.clear();
        }

        for (auto& iter : partial_index) {
            auto& new_cluster = iter.second;
            if (new_cluster.size() <= 1 || iter.first == std::vector<int>{kSingletonValueId}) {
                singletons.push_back(std::move(cluster));
                continue;
            }

            new_size += new_cluster.size();
            new_key_gap += new_cluster.size() * log(new_cluster.size());
            new_nep += CalculateNep(new_cluster.size());

            new_index.emplace_back(std::move(new_cluster));
        }
        partial_index.clear();
    }

    double new_entropy = log(this->relation_size_) - new_key_gap / this->relation_size_;

    SortClusters(new_index);

    return std::make_unique<PLIWithSingletons>(std::move(new_index), std::move(singletons),
                                               new_size, new_entropy, new_nep, this->relation_size_,
                                               this->relation_size_);
}

std::unique_ptr<PLIWithSingletons> PLIWithSingletons::Intersect(
        PLIWithSingletons const* that) const {
    if (this->relation_size_ != that->relation_size_)
        throw std::invalid_argument("different size of relations");

    if (this->size_ > that->size_) {
        return that->Probe(this->CalculateAndGetProbingTable());
    }
    return this->Probe(that->CalculateAndGetProbingTable());
}

}  // namespace model
