#include "core/algorithms/dd/lattice/lattice_algo.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <limits>
#include <list>
#include <numeric>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/model/table/column_index.h"
#include "core/model/types/numeric_type.h"
#include "core/util/levenshtein_distance.h"
#include "core/util/logger.h"

namespace algos::dd {

LatticeAlgorithm::LatticeAlgorithm() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void LatticeAlgorithm::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    config::InputTable default_table;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&difference_table_, kDifferenceTable, kDDifferenceTable, default_table});
    RegisterOption(Option{&num_rows_, kNumRows, kDNumRows, 0U});
    RegisterOption(Option{&num_columns_, kNumColumns, kDNumColumns, 0U});
    RegisterOption(Option{&satisfaction_threshold_, kError, kDError, 0.0});
    RegisterOption(Option{&support_threshold_, kMinSupport, kDMinSupport, 0.0});
}

void LatticeAlgorithm::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kDifferenceTable, kNumRows, kNumColumns, kError, kMinSupport});
}

void LatticeAlgorithm::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_,
                                                                       false);  // nulls are ignored
    if (typed_relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: ADD mining is meaningless.");
    }
}

void LatticeAlgorithm::SetLimits() {
    unsigned all_rows_num = typed_relation_->GetNumRows();
    model::ColumnIndex all_columns_num = typed_relation_->GetNumColumns();
    if (num_rows_ > all_rows_num) {
        throw std::invalid_argument(
                "'num_rows' must be less or equal to the number of rows in the table (total "
                "rows: " +
                std::to_string(all_rows_num) + ")");
    }
    if (num_columns_ > all_columns_num) {
        throw std::invalid_argument(
                "'num_columns' must be less or equal to the number of columns in the table (total "
                "columns: " +
                std::to_string(all_columns_num) + ")");
    }
    if (num_rows_ == 0) num_rows_ = all_rows_num;
    if (num_columns_ == 0) num_columns_ = all_columns_num;
}

void LatticeAlgorithm::CheckTypes() {
    type_ids_.resize(num_columns_, model::TypeId::kUndefined);
    for (model::ColumnIndex column_index = 0; column_index < num_columns_; column_index++) {
        model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);
        model::TypeId type_id = column.GetTypeId();

        if (type_id == +model::TypeId::kUndefined) {
            throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                        "\" type undefined.");
        }
        if (type_id == +model::TypeId::kMixed) {
            throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                        "\" contains values of different types.");
        }

        type_ids_[column_index] = type_id;

        for (std::size_t row_index = 0; row_index < num_rows_; row_index++) {
            if (column.IsNull(row_index)) {
                throw std::runtime_error("Some of the value coordinates are nulls.");
            }
            if (column.IsEmpty(row_index)) {
                throw std::runtime_error("Some of the value coordinates are empty.");
            }
        }
    }
}

void LatticeAlgorithm::ParseDifferenceTable() {
    if (difference_table_) {
        difference_typed_relation_ =
                model::ColumnLayoutTypedRelationData::CreateFrom(*difference_table_,
                                                                 false);  // nulls are ignored
        if (typed_relation_->GetNumColumns() != num_columns_) {
            throw std::invalid_argument(
                    "The number of columns in the difference table must be equal to the number of "
                    "columns in the loaded table or to 'num_columns' if specified");
        }
    }
}

unsigned long long LatticeAlgorithm::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();
    LOG_DEBUG("Start");

    SetLimits();
    CheckTypes();
    ParseDifferenceTable();

    CalculateAllDistances();
    CalculateIndexSearchSpaces();

    LOG_INFO("Calculated distances");
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG_DEBUG("Current time: {}", elapsed_milliseconds.count());

    CalculateTuplePairs();

    minDD();

    LOG_INFO("Calculated ADDs");
    elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG_DEBUG("Current time: {}", elapsed_milliseconds.count());

    CollectPaths();

    LOG_INFO("Extracted ADDs");
    elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG_DEBUG("Current time: {}", elapsed_milliseconds.count());

    PrintResults();

    elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG_INFO("Algorithm time: {}", elapsed_milliseconds.count());
    return elapsed_milliseconds.count();
}

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
    min_max_dif_.resize(num_columns_, {0, 0});

    for (model::ColumnIndex column_index = 0; column_index < num_columns_; column_index++) {
        DistancePositionListIndex pli(typed_relation_->GetColumnData(column_index), num_rows_);
        std::vector<ClusterInfo> const& clusters = pli.GetClusters();
        std::size_t const num_clusters = clusters.size();
        std::vector<std::vector<double>> cur_column_distances;
        cur_column_distances.reserve(num_clusters);

        double max_dif = 0, min_dif = std::numeric_limits<double>::max();
        for (ClusterIndex i = 0; i < num_clusters; i++) {
            cur_column_distances.emplace_back();
            cur_column_distances[i].reserve(num_clusters - i);
            cur_column_distances[i].push_back(0);
            for (ClusterIndex j = i + 1; j < num_clusters; j++) {
                std::size_t const first_index = clusters[i].first_tuple_index;
                std::size_t const second_index = clusters[j].first_tuple_index;
                double const dif = CalculateDistance(column_index, {first_index, second_index});
                max_dif = std::max(max_dif, dif);
                min_dif = std::min(min_dif, dif);
                cur_column_distances[i].push_back(dif);
            }
            if (clusters[i].size > 1) min_dif = 0;
        }
        min_max_dif_[column_index] = {min_dif, max_dif};
        distances_.emplace_back(std::move(cur_column_distances));
        plis_[column_index] = std::move(pli);
    }
}

std::vector<DFConstraint> LatticeAlgorithm::IndexSearchSpace(model::ColumnIndex index) {
    std::vector<DFConstraint> dfs;

    std::size_t dif_num_rows = difference_typed_relation_->GetNumRows();

    model::TypedColumnData const& dif_column = difference_typed_relation_->GetColumnData(index);

    // accepts a string in the following format: [a;b], where a and b are double type values
    boost::regex df_regex(R"(\[(.*)\;(.*)\]$)");
    boost::regex double_regex(
            R"(^[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$|)"
            R"(^[+-]?(?i)(inf|nan)(?-i)$|)"
            R"(^[+-]?0[xX](((\d|[a-f]|[A-F]))+(\.(\d|[a-f]|[A-F])*)?|\.(\d|[a-f]|[A-F])+)([pP][+-]?\d+)?$)");

    for (std::size_t row_index = 0; row_index < dif_num_rows; row_index++) {
        model::TypeId type_id = dif_column.GetValueTypeId(row_index);
        if (type_id == +model::TypeId::kString) {
            std::string df_str = dif_column.GetDataAsString(row_index);
            boost::smatch matches;
            if (boost::regex_match(df_str, matches, df_regex)) {
                if (boost::regex_match(matches[1].str(), double_regex) &&
                    boost::regex_match(matches[2].str(), double_regex)) {
                    double const lower_limit =
                            model::TypeConverter<double>::kConvert(matches[1].str());
                    double const upper_limit =
                            model::TypeConverter<double>::kConvert(matches[2].str());

                    model::DFConstraint parsed_limits{lower_limit, upper_limit};

                    if (parsed_limits.IsValid()) {
                        auto intersect = parsed_limits.IntersectWith(min_max_dif_[index]);
                        if (intersect && *intersect != min_max_dif_[index]) {
                            dfs.push_back(*intersect);
                        }
                    }
                }
            }
        }
    }
    return dfs;
}

void LatticeAlgorithm::CalculateIndexSearchSpaces() {
    std::vector<DFConstraint> new_min_max_dif;
    std::vector<std::vector<std::vector<double>>> new_distances;
    std::vector<DistancePositionListIndex> new_plis;
    new_min_max_dif.reserve(num_columns_);
    new_distances.reserve(num_columns_);
    new_plis.reserve(num_columns_);
    for (model::ColumnIndex index = 0; index < num_columns_; index++) {
        std::vector<DFConstraint> cur_index_search_space = IndexSearchSpace(index);
        if (!cur_index_search_space.empty()) {
            index_search_spaces_.push_back(std::move(cur_index_search_space));
            non_empty_cols_.push_back(index);
            new_min_max_dif.push_back(min_max_dif_[index]);
            new_distances.push_back(std::move(distances_[index]));
            new_plis.push_back(std::move(plis_[index]));
        }
    }
    num_columns_ = non_empty_cols_.size();
    min_max_dif_ = std::move(new_min_max_dif);
    distances_ = std::move(new_distances);
    plis_ = std::move(new_plis);
}

// must be inline for optimization (gcc 11.4.0)
inline bool LatticeAlgorithm::CheckDFConstraint(DFConstraint const& dif_constraint,
                                                model::ColumnIndex column_index,
                                                std::pair<std::size_t, std::size_t> tuple_pair) {
    std::vector<ClusterIndex> const& cur_index = plis_[column_index].GetInvertedIndex();
    ClusterIndex const first_cluster = cur_index[tuple_pair.first];
    ClusterIndex const second_cluster = cur_index[tuple_pair.second];
    ClusterIndex const min_cluster = std::min(first_cluster, second_cluster);
    ClusterIndex const max_cluster = std::max(first_cluster, second_cluster);
    double const dif = distances_[column_index][min_cluster][max_cluster - min_cluster];

    if (type_ids_[column_index] == +model::TypeId::kDouble) {
        return dif_constraint.Contains(dif);
    }

    return dif >= dif_constraint.lower_bound && dif <= dif_constraint.upper_bound;
}

void LatticeAlgorithm::CalculateTuplePairs() {
    df_num_ = std::accumulate(
            index_search_spaces_.begin(), index_search_spaces_.end(), 0,
            [](std::size_t acc, auto const& search_space) { return acc + search_space.size(); });

    base_partitions_.resize(num_columns_);
    df_column_.reserve(df_num_);
    column_begin_idx_.reserve(num_columns_);
    tuple_pair_num_ = num_rows_ * (num_rows_ - 1) / 2;
    tuple_pair_threshold_ = tuple_pair_num_ * support_threshold_;

    for (model::ColumnIndex column_index = 0; column_index < num_columns_; column_index++) {
        auto const& search_space = index_search_spaces_[column_index];
        auto& column_partitions = base_partitions_[column_index];
        column_partitions.resize(search_space.size(), make_bitset(tuple_pair_num_));

        column_begin_idx_.push_back(df_column_.size());
        for (std::size_t df_index = 0; df_index < search_space.size(); ++df_index) {
            df_column_.push_back(column_index);

            std::size_t pair_index = 0;
            for (std::size_t first_index = 0; first_index < num_rows_; first_index++) {
                for (std::size_t second_index = first_index + 1; second_index < num_rows_;
                     second_index++) {
                    if (CheckDFConstraint(search_space[df_index], column_index,
                                          {first_index, second_index}))
                        set(column_partitions[df_index], pair_index);
                    else
                        reset(column_partitions[df_index], pair_index);
                    pair_index++;
                }
            }
        }
    }
}

void LatticeAlgorithm::BuildFirstLevel() {
    current_level_.clear();
    std::size_t df_index = 0;
    for (model::ColumnIndex column_index = 0; column_index < num_columns_; column_index++) {
        auto const& column_partitions = base_partitions_[column_index];
        for (std::size_t partition_index = 0; partition_index < column_partitions.size();
             ++partition_index) {
            auto new_node = std::make_unique<LatticeNode>();
            resize(new_node->df_, df_num_);
            set(new_node->df_, df_index);
            new_node->partition_ = column_partitions[partition_index];
            current_level_.push_back(std::move(new_node));
            ++df_index;
        }
    }
}

void LatticeAlgorithm::BuildNextLevel() {
    next_level_.clear();
    std::unordered_set<Bitset, BitsetHash> already_added;
    for (std::size_t i = 0; i < current_level_.size(); ++i) {
        for (std::size_t j = i + 1; j < current_level_.size(); ++j) {
            auto& node1 = *current_level_[i];
            auto& node2 = *current_level_[j];

            Bitset new_partition = bit_and(node1.partition_, node2.partition_);
            if (bit_count(new_partition) < tuple_pair_threshold_) continue;

            Bitset diff_bits = bit_xor(node1.df_, node2.df_);
            Bitset union_bits = bit_or(node1.df_, node2.df_);
            if (!already_added.insert(union_bits).second) continue;

            if (bit_count(union_bits) == bit_count(node1.df_) + 1) {
                size_t diff_df1 = find_first(diff_bits);
                size_t diff_df2 = find_next(diff_bits, diff_df1);
                if (df_column_[diff_df1] != df_column_[diff_df2]) {
                    DDSet new_dds = node1.dds_;
                    new_dds->insert((node2.dds_)->begin(), (node2.dds_)->end());

                    bool node_reducible = false;
                    for (auto const& dd : *new_dds)
                        if (bit_none(bit_and(dd, bit_not(union_bits)))) {
                            node_reducible = true;
                            break;
                        }
                    if (node_reducible) continue;

                    auto new_node =
                            std::make_unique<LatticeNode>(union_bits, new_partition, new_dds);
                    next_level_.push_back(std::move(new_node));
                }
            }
        }
    }
    current_level_ = std::move(next_level_);
}

void LatticeAlgorithm::FindRhs(Bitset const& df_partition, model::ColumnIndex col,
                               Bitset& col_intervals) {
    auto const& column_partitions = base_partitions_[col];
    std::size_t const column_partitions_count = column_partitions.size();
    std::size_t const column_begin_idx = column_begin_idx_[col];
    std::size_t const df_partition_size = bit_count(df_partition);

    double lhs_rhs_df_matches = 0.0;
    for (std::size_t idx = 1; idx <= column_partitions_count; ++idx) {
        lhs_rhs_df_matches +=
                bit_count(bit_and(df_partition, column_partitions[column_partitions_count - idx]));

        if (lhs_rhs_df_matches >= satisfaction_threshold_ * df_partition_size) break;
        reset(col_intervals, column_begin_idx + column_partitions_count - idx);
    }

    for (std::size_t idx = 0; idx < column_partitions_count; ++idx) {
        lhs_rhs_df_matches = bit_count(bit_and(df_partition, column_partitions[idx]));

        if (lhs_rhs_df_matches > 0) break;
        reset(col_intervals, column_begin_idx + idx);
    }
}

bool LatticeAlgorithm::SameSubtrees(DFTreeNode* subtree_a, DFTreeNode* subtree_b) {
    auto const& children_a = subtree_a->left_children_;
    auto const& children_b = subtree_b->left_children_;

    if (children_a.size() != children_b.size()) return false;

    for (auto const& [idxA, child_a] : children_a) {
        auto it_b = children_b.find(idxA);
        if (it_b == children_b.end()) return false;

        auto const& child_b = it_b->second;
        if (child_a->right_idx_ != child_b->right_idx_) return false;

        if (!SameSubtrees(child_a.get(), child_b.get())) return false;
    }
    return true;
}

void LatticeAlgorithm::Combine(DFTreeNode* root, Bitset const& lhs) {
    auto* curr_node = root;
    DFIdx curr_df = find_first(lhs);

    while (curr_df != NPOS) {
        auto& node_children = curr_node->left_children_;
        auto it = node_children.lower_bound(curr_df);

        if (it != node_children.begin()) {
            auto prev_df = std::prev(it);

            if (df_column_[prev_df->first] == df_column_[curr_df]) {
                auto* curr_df_node = node_children.at(curr_df).get();
                auto* prev_df_node = prev_df->second.get();

                if (SameSubtrees(curr_df_node, prev_df_node)) {
                    prev_df_node->right_idx_ = curr_df;
                    node_children.erase(curr_df);
                    return;
                }
            }
        }

        curr_node = node_children.at(curr_df).get();
        curr_df = find_next(lhs, curr_df);
    }
}

void LatticeAlgorithm::CheckAndCombine(DFTreeNode* root, Bitset const& lhs) {
    auto* curr_node = root;
    DFIdx curr_df = find_first(lhs);

    while (!curr_node->left_children_.empty() || !curr_node->left_idx_.has_value()) {
        auto it = curr_node->left_children_.find(curr_df);
        if (it == curr_node->left_children_.end()) break;

        curr_node = it->second.get();
        curr_df = find_next(lhs, curr_df);
    }

    if (curr_node->left_children_.empty() && curr_node->left_idx_.has_value()) return;

    while (curr_df != NPOS) {
        auto new_node = std::make_unique<DFTreeNode>(curr_df);
        curr_node =
                curr_node->left_children_.emplace(curr_df, std::move(new_node)).first->second.get();
        curr_df = find_next(lhs, curr_df);
    }

    Combine(root, lhs);
}

void LatticeAlgorithm::minDD() {
    current_level_.reserve(10000);
    next_level_.reserve(10000);
    for (std::size_t i = 0; i < num_columns_; ++i) {
        if (i == 0)
            BuildFirstLevel();
        else
            BuildNextLevel();

        for (auto node_iterator = current_level_.begin(); node_iterator != current_level_.end();) {
            auto const& node_df = (*node_iterator)->df_;
            auto const& node_partition = (*node_iterator)->partition_;
            auto& node_dds = (*node_iterator)->dds_;

            for (size_t B = 0; B < num_columns_; ++B) {
                Bitset B_interval = make_bitset(df_num_);
                std::size_t col_first_df = column_begin_idx_[B];
                std::size_t col_partition_size = base_partitions_[B].size();
                for (size_t i = 0; i < col_partition_size; ++i) set(B_interval, col_first_df + i);

                if (bit_none(bit_and(B_interval, node_df))) {
                    FindRhs(node_partition, B, B_interval);

                    if (bit_none(B_interval)) continue;
                    auto* DD_tree = trees_.try_emplace(B_interval, std::make_unique<DFTreeNode>())
                                            .first->second.get();
                    CheckAndCombine(DD_tree, node_df);
                    if (bit_count(B_interval) == 1) node_dds->insert(bit_or(B_interval, node_df));
                }
            }
            ++node_iterator;
        }
    }
}

DFStringConstraint LatticeAlgorithm::MakeDF(DFIdx left_idx, DFIdx right_idx) {
    model::ColumnIndex column_index = df_column_[left_idx];
    DFIdx column_begin = column_begin_idx_[column_index];

    double lower = index_search_spaces_[column_index][left_idx - column_begin].lower_bound;
    double upper = index_search_spaces_[column_index][right_idx - column_begin].upper_bound;
    DFStringConstraint df(input_table_->GetColumnName(non_empty_cols_[column_index]), lower, upper);

    return df;
}

DFStringConstraint LatticeAlgorithm::MakeDF(Bitset const& bitset_df) {
    DFIdx left_idx = find_first(bitset_df);

    DFIdx right_idx;
    for (size_t i = left_idx; i != NPOS; i = find_next(bitset_df, i)) right_idx = i;

    return MakeDF(left_idx, right_idx);
}

void LatticeAlgorithm::TreeNodeDFS(DFTreeNode* node, DFStringList const& rhs, DFStringList& lhs) {
    if (node->left_children_.empty()) {
        DDs_.emplace_back(lhs, rhs);
        return;
    }
    for (auto const& [_, child] : node->left_children_) {
        if (!child->left_idx_ || !child->right_idx_) continue;
        DFStringConstraint lhs_df = MakeDF(child->left_idx_.value(), child->right_idx_.value());
        lhs.push_back(lhs_df);
        TreeNodeDFS(child.get(), rhs, lhs);
        lhs.pop_back();
    }
}

void LatticeAlgorithm::CollectPaths() {
    for (auto const& [rhs_bitset, root] : trees_) {
        DFStringList rhs{MakeDF(rhs_bitset)};
        DFStringList lhs;
        TreeNodeDFS(root.get(), rhs, lhs);
    }
}

void LatticeAlgorithm::PrintResults() {
    LOG_INFO("Minimal cover size: {}", DDs_.size());
    for (auto const& result_str : DDs_) {
        LOG_DEBUG("{}", result_str.ToString());
    }
}

std::list<DDString> LatticeAlgorithm::GetDDStringList() const {
    return DDs_;
}

}  // namespace algos::dd