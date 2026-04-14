#include "core/algorithms/dd/lattice/lattice_algo.h"

#include <algorithm>
#include <cstddef>
#include <unordered_set>
#include <vector>

#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/model/types/numeric_type.h"
#include "core/util/logger.h"

namespace algos::dd {

double LatticeAlgorithm::CalculateDistance(model::ColumnIndex column_index,
                                           std::pair<std::size_t, std::size_t> tuple_pair) {
    model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);

    double dif = 0;
    if (column.GetType().IsMetrizable()) {
        std::byte const* first_value = column.GetValue(tuple_pair.first);
        std::byte const* second_value = column.GetValue(tuple_pair.second);
        auto const& type = static_cast<model::IMetrizableType const&>(column.GetType());
        dif = type.Dist(first_value, second_value);
    }
    return dif;
}

void LatticeAlgorithm::CalculateAllDistances() {
    plis_.resize(num_columns_);
    distances_.reserve(num_columns_);

    for (model::ColumnIndex column_index = 0; column_index < num_columns_; column_index++) {
        DistancePositionListIndex pli(typed_relation_->GetColumnData(column_index), num_rows_);
        std::vector<ClusterInfo> const& clusters = pli.GetClusters();
        std::size_t const num_clusters = clusters.size();
        std::vector<std::vector<double>> cur_column_distances;
        cur_column_distances.reserve(num_clusters);

        for (ClusterIndex i = 0; i < num_clusters; i++) {
            cur_column_distances.emplace_back();
            cur_column_distances[i].reserve(num_clusters - i);
            cur_column_distances[i].push_back(0);
            for (ClusterIndex j = i + 1; j < num_clusters; j++) {
                std::size_t const first_index = clusters[i].first_tuple_index;
                std::size_t const second_index = clusters[j].first_tuple_index;
                double const dif = CalculateDistance(column_index, {first_index, second_index});
                cur_column_distances[i].push_back(dif);
            }
        }
        distances_.emplace_back(std::move(cur_column_distances));
        plis_[column_index] = std::move(pli);
    }
}

void LatticeAlgorithm::CalculateBasePartitions() {
    tuple_pair_num_ = num_rows_ * (num_rows_ - 1) / 2;

    std::vector<std::pair<std::size_t, std::size_t>> all_pairs;
    all_pairs.reserve(tuple_pair_num_);
    for (std::size_t first = 0; first < num_rows_; ++first)
        for (std::size_t second = first + 1; second < num_rows_; ++second)
            all_pairs.emplace_back(first, second);

    std::vector<std::vector<double>> column_distances(num_columns_);
    for (model::ColumnIndex col = 0; col < num_columns_; ++col) {
        column_distances[col].resize(tuple_pair_num_);

        for (std::size_t pair_idx = 0; pair_idx < tuple_pair_num_; ++pair_idx) {
            auto const& pair = all_pairs[pair_idx];

            auto const& inverted_index = plis_[col].GetInvertedIndex();
            ClusterIndex first_cluster = inverted_index[pair.first];
            ClusterIndex second_cluster = inverted_index[pair.second];

            ClusterIndex min_cluster = std::min(first_cluster, second_cluster);
            ClusterIndex max_cluster = std::max(first_cluster, second_cluster);

            double distance;
            if (min_cluster == max_cluster)
                distance = 0.0;
            else
                distance = distances_[col][min_cluster][max_cluster - min_cluster];
            column_distances[col][pair_idx] = distance;
        }
    }

    base_partitions_.resize(df_num_);
    std::size_t interval_idx = 0;

    for (model::ColumnIndex col = 0; col < num_columns_; ++col) {
        auto const& intervals = base_intervals_[col];

        for (auto const& interval : intervals) {
            base_partitions_[interval_idx].resize(tuple_pair_num_);

            for (std::size_t pair_idx = 0; pair_idx < tuple_pair_num_; ++pair_idx) {
                double distance = column_distances[col][pair_idx];
                bool in_interval = distance >= interval.first && distance <= interval.second;

                base_partitions_[interval_idx][pair_idx] = in_interval;
            }
            interval_idx++;
        }
    }
}

void LatticeAlgorithm::BuildFirstLevel() {
    for (size_t i = 0; i < df_num_; ++i) {
        auto new_node = std::make_shared<LatticeNode>();
        (new_node->df_).resize(df_num_);
        new_node->df_.set(i);
        new_node->partition_ = base_partitions_[i];

        current_level_.push_back(new_node);
    }
}

void LatticeAlgorithm::BuildNextLevel() {
    for (std::size_t i = 0; i < current_level_.size(); ++i) {
        for (std::size_t j = i + 1; j < current_level_.size(); ++j) {
            auto node1 = current_level_[i];
            auto node2 = current_level_[j];

            Bitset union_bits = node1->df_ | node2->df_;
            Bitset diff_bits = node1->df_ ^ node2->df_;

            if (union_bits.count() == node1->df_.count() + 1) {
                size_t diff_df1 = diff_bits.find_first();
                size_t diff_df2 = diff_bits.find_next(diff_df1);
                if (column_for_df_[diff_df1] != column_for_df_[diff_df2]) {
                    Bitset new_partition = node1->partition_ & node2->partition_;

                    std::unordered_set<Bitset> new_dds = node1->dds_;
                    new_dds.insert((node2->dds_).begin(), (node2->dds_).end());

                    auto new_node =
                            std::make_shared<LatticeNode>(union_bits, new_partition, new_dds);
                    next_level_.push_back(new_node);
                }
            }
        }
    }
    current_level_ = std::move(next_level_);
    next_level_.clear();
}

std::pair<double, double> LatticeAlgorithm::FindRhs(Bitset const& df_partition,
                                                    model::ColumnIndex col) {
    auto const [first_idx, last_idx] = column_partition_idxs_[col];
    std::size_t const df_partition_size = df_partition.count();
    auto const& column_intervals_idxs = base_intervals_[col];

    double lhs_rhs_df_matches = 0.0;
    double right = column_intervals_idxs.back().second;
    for (std::size_t idx = last_idx; idx >= first_idx; --idx) {
        lhs_rhs_df_matches += (df_partition & base_partitions_[idx]).count();

        if (lhs_rhs_df_matches / df_partition_size > 1 - satisfaction_threshold_) {
            right = column_intervals_idxs[idx - first_idx].second;
            break;
        }
    }

    double left = column_intervals_idxs.front().first;
    for (std::size_t idx = first_idx; idx <= last_idx; ++idx) {
        lhs_rhs_df_matches = (df_partition & base_partitions_[idx]).count();

        if (lhs_rhs_df_matches > 0) {
            left = column_intervals_idxs[idx - first_idx].first;
            break;
        }
    }

    return {left, right};
}

bool LatticeAlgorithm::SameSubtrees(std::shared_ptr<DFTreeNode> subtree_a,
                                    std::shared_ptr<DFTreeNode> subtree_b) {
    auto const& children_a = subtree_a->left_children_;
    auto const& children_b = subtree_b->left_children_;

    if (children_a.size() != children_b.size()) return false;

    for (auto const& [idxA, child_a] : children_a) {
        auto it_b = children_b.find(idxA);
        if (it_b == children_b.end()) return false;

        auto const child_b = it_b->second;
        if (child_a->right_idx_ != child_b->right_idx_) return false;

        if (!SameSubtrees(child_a, child_b)) return false;
    }
    return true;
}

void LatticeAlgorithm::Combine(std::shared_ptr<DFTreeNode> root, Bitset const& lhs) {
    auto curr_node = root;
    DFIdx curr_df = lhs.find_first();

    while (curr_df != Bitset::npos) {
        auto& node_children = curr_node->left_children_;
        auto it = node_children.lower_bound(curr_df);

        if (it != node_children.begin()) {
            auto prev_df = std::prev(it);

            if (column_for_df_[prev_df->first] == column_for_df_[curr_df]) {
                auto curr_df_node = node_children.at(curr_df);
                auto prev_df_node = prev_df->second;

                if (SameSubtrees(curr_df_node, prev_df_node)) {
                    prev_df_node->right_idx_ = curr_df;
                    node_children.erase(curr_df);
                    return;
                }
            }
        }

        curr_node = node_children.at(curr_df);
        curr_df = lhs.find_next(curr_df);
    }
}

void LatticeAlgorithm::CheckAndCombine(DFTree& tree, Bitset const& lhs) {
    auto curr_node = tree.root_;
    DFIdx curr_df = lhs.find_first();

    while (!curr_node->left_children_.empty() || !curr_node->left_idx_.has_value()) {
        auto it = curr_node->left_children_.find(curr_df);
        if (it == curr_node->left_children_.end()) break;

        curr_node = it->second;
        curr_df = lhs.find_next(curr_df);
    }

    if (curr_node->left_children_.empty() && curr_node->left_idx_.has_value()) return;

    while (curr_df != Bitset::npos) {
        auto new_node = std::make_shared<DFTreeNode>(curr_df);
        curr_node = curr_node->left_children_.emplace(curr_df, std::move(new_node)).first->second;
        curr_df = lhs.find_next(curr_df);
    }

    Combine(tree.root_, lhs);
}

}  // namespace algos::dd