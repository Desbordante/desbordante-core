#include "algorithms/dd/split/split.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <deque>
#include <limits>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/regex.hpp>
#include <easylogging++.h>

#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "model/table/column_index.h"
#include "model/types/numeric_type.h"
#include "util/levenshtein_distance.h"

namespace algos::dd {

Split::Split() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void Split::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    config::InputTable default_table;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&difference_table_, kDifferenceTable, kDDifferenceTable, default_table});
    RegisterOption(Option{&num_rows_, kNumRows, kDNumRows, 0U});
    RegisterOption(Option{&num_columns_, kNumColumns, kDNUmColumns, 0U});
}

void Split::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kDifferenceTable, kNumRows, kNumColumns});
}

void Split::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, false);  // nulls are
                                                                             // ignored
    input_table_->Reset();
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_,
                                                                       false);  // nulls are ignored
    if (typed_relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: DD mining is meaningless.");
    }
}

void Split::SetLimits() {
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

void Split::ParseDifferenceTable() {
    has_dif_table_ = (difference_table_.get() != nullptr);

    if (has_dif_table_) {
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

unsigned long long Split::ExecuteInternal() {
    SetLimits();
    ParseDifferenceTable();

    auto const start_time = std::chrono::system_clock::now();
    LOG(DEBUG) << "Start";

    CalculateAllDistances();
    CalculateIndexSearchSpaces();

    if (reduce_method_ == +Reduce::IEHybrid) {
        CalculateTuplePairs();
    }

    LOG(INFO) << "Calculated distances";
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG(DEBUG) << "Current time: " << elapsed_milliseconds.count();
    LOG(INFO) << "Minimum and maximum distances for each column with non-empty search space:";

    for (model::ColumnIndex index = 0; index < num_columns_; index++)
        LOG(INFO) << input_table_->GetColumnName(non_empty_cols_[index]) << ": "
                  << min_max_dif_[index].lower_bound << ", " << min_max_dif_[index].upper_bound;

    unsigned const search_size = ReduceDDs(start_time);

    LOG(DEBUG) << "Reduced dependencies";

    unsigned num_cycles = RemoveRedundantDDs();

    LOG(INFO) << "Removed redundant dependencies";
    LOG(DEBUG) << "Cycles: " << num_cycles;

    num_cycles = RemoveTransitiveDDs();

    LOG(INFO) << "Removed transitive dependencies";
    LOG(DEBUG) << "Cycles: " << num_cycles;
    LOG(INFO) << "Search space size: " << search_size;

    PrintResults();

    elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG(INFO) << "Algorithm time: " << elapsed_milliseconds.count();
    return elapsed_milliseconds.count();
}

unsigned Split::ReduceDDs(auto const& start_time) {
    unsigned search_size = 0;
    std::vector<DF> search, dfs_y;
    std::list<DD> reduced;

    for (model::ColumnIndex index = 0; index < num_columns_; index++) {
        std::vector<model::ColumnIndex> indices;
        for (model::ColumnIndex j = 0; j < num_columns_; j++) {
            if (j != index) indices.emplace_back(j);
        }

        search = SearchSpace(indices);
        dfs_y = SearchSpace(index);
        search_size += search.size() * (dfs_y.size() - 1);
        LOG(DEBUG) << "Calculated search spaces for column "
                   << input_table_->GetColumnName(non_empty_cols_[index]);
        auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - start_time);
        LOG(DEBUG) << "Current time: " << elapsed_milliseconds.count();
        LOG(DEBUG) << "Lhs and rhs search space sizes: " << search.size() << " " << dfs_y.size();
        LOG(DEBUG) << "Number of verifications for each df in rhs:";

        for (auto& df_y : dfs_y) {
            unsigned cnt = 0;
            if (df_y != dfs_y.front()) {
                switch (reduce_method_) {
                    case +Reduce::Negative:
                        reduced = NegativePruningReduce(df_y, search, cnt);
                        break;
                    case +Reduce::Hybrid:
                        reduced = HybridPruningReduce(df_y, search, cnt);
                        break;
                    case +Reduce::IEHybrid:
                        reduced = InstanceExclusionReduce(tuple_pairs_, search, df_y, cnt);
                        break;
                    default:
                        break;
                }
                dd_collection_.splice(dd_collection_.end(), reduced);
            }
            LOG(DEBUG) << cnt;
        }
        elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - start_time);
        LOG(INFO) << "Reduced dependencies with their rhs on column "
                  << input_table_->GetColumnName(non_empty_cols_[index]);
        LOG(DEBUG) << "Current time: " << elapsed_milliseconds.count();
    }
    return search_size;
}

unsigned Split::RemoveRedundantDDs() {
    unsigned num_cycles = 0;
    std::list<DD> dd_collection_copy;

    while (true) {
        num_cycles++;
        dd_collection_copy.clear();
        std::size_t left_dd_index = 0;
        for (auto& left_dd : dd_collection_) {
            bool is_redundant = false;
            std::size_t right_dd_index = 0;
            for (auto& right_dd : dd_collection_) {
                if (left_dd_index != right_dd_index) {
                    if (Subsume(right_dd.lhs, left_dd.lhs)) {
                        if (Subsume(left_dd.rhs, right_dd.rhs)) {
                            is_redundant = true;
                            break;
                        }
                    }
                }
                right_dd_index++;
            }
            if (!is_redundant) dd_collection_copy.push_back(left_dd);
            left_dd_index++;
        }
        if (dd_collection_copy.size() == dd_collection_.size()) break;
        dd_collection_ = dd_collection_copy;
    }
    return num_cycles;
}

unsigned Split::RemoveTransitiveDDs() {
    unsigned num_cycles = 0;
    std::list<DD> results_copy;

    while (true) {
        num_cycles++;
        results_copy.clear();
        bool is_removable = false;
        for (auto& dd3 : dd_collection_) {
            bool remove = false;
            for (auto& dd1 : dd_collection_) {
                for (auto& dd2 : dd_collection_) {
                    if (Subsume(dd2.lhs, dd1.rhs) && dd1.lhs == dd3.lhs && dd2.rhs == dd3.rhs) {
                        if (!is_removable) remove = true;
                        is_removable = true;
                        break;
                    }
                }
                if (is_removable) break;
            }
            if (!remove) results_copy.push_back(dd3);
        }
        if (results_copy.size() == dd_collection_.size()) break;
        dd_collection_ = results_copy;
    }
    return num_cycles;
}

void Split::CalculateIndexSearchSpaces() {
    std::vector<DFConstraint> new_min_max_dif;
    std::vector<std::vector<std::vector<double>>> new_distances;
    new_min_max_dif.reserve(num_columns_);
    new_distances.reserve(num_columns_);
    for (model::ColumnIndex index = 0; index < num_columns_; index++) {
        std::vector<DFConstraint> cur_index_search_space = IndexSearchSpace(index);
        if (!cur_index_search_space.empty()) {
            index_search_spaces_.push_back(std::move(cur_index_search_space));
            non_empty_cols_.push_back(index);
            new_min_max_dif.push_back(min_max_dif_[index]);
            new_distances.push_back(std::move(distances_[index]));
        }
    }
    num_columns_ = non_empty_cols_.size();
    min_max_dif_ = std::move(new_min_max_dif);
    distances_ = std::move(new_distances);
}

double Split::CalculateDistance(model::ColumnIndex column_index,
                                std::pair<std::size_t, std::size_t> tuple_pair) {
    model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);
    model::TypeId type_id = column.GetTypeId();

    if (type_ids_[column_index] == +model::TypeId::kUndefined) {
        type_ids_[column_index] = type_id;
    }

    if (type_id == +model::TypeId::kUndefined) {
        throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                    "\" type undefined.");
    }
    if (type_id == +model::TypeId::kMixed) {
        throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                    "\" contains values of different types.");
    }
    if (column.IsNull(tuple_pair.first) || column.IsNull(tuple_pair.second)) {
        throw std::runtime_error("Some of the value coordinates are nulls.");
    }
    if (column.IsEmpty(tuple_pair.first) || column.IsEmpty(tuple_pair.second)) {
        throw std::runtime_error("Some of the value coordinates are empty.");
    }
    double dif = 0;
    if (column.GetType().IsMetrizable()) {
        std::byte const* first_value = column.GetValue(tuple_pair.first);
        std::byte const* second_value = column.GetValue(tuple_pair.second);
        auto const& type = static_cast<model::IMetrizableType const&>(column.GetType());
        dif = type.Dist(first_value, second_value);
    }
    return dif;
}

// must be inline for optimization (gcc 11.4.0)
inline bool Split::CheckDF(DF const& dif_func, std::pair<std::size_t, std::size_t> tuple_pair) {
    for (model::ColumnIndex column_index = 0; column_index < num_columns_; column_index++) {
        double const dif = distances_[column_index][tuple_pair.first][tuple_pair.second];

        if (type_ids_[column_index] == +model::TypeId::kDouble) {
            if (!dif_func[column_index].Contains(dif)) {
                return false;
            }
        } else {
            if (dif < dif_func[column_index].lower_bound ||
                dif > dif_func[column_index].upper_bound) {
                return false;
            }
        }
    }
    return true;
}

bool Split::VerifyDD(DF const& lhs, DF const& rhs) {
    for (std::size_t i = 0; i < num_rows_; i++) {
        for (std::size_t j = i + 1; j < num_rows_; j++) {
            if (CheckDF(lhs, {i, j}) && !CheckDF(rhs, {i, j})) return false;
        }
    }
    return true;
}

void Split::InsertDistance(model::ColumnIndex column_index, std::size_t first_index,
                           std::size_t second_index, double& min_dif, double& max_dif) {
    if (first_index < second_index) {
        double const dif = CalculateDistance(column_index, {first_index, second_index});
        max_dif = std::max(max_dif, dif);
        min_dif = std::min(min_dif, dif);
        distances_[column_index][first_index][second_index] = dif;
    } else if (first_index == second_index) {
        distances_[column_index][first_index][second_index] = 0;
    } else {
        distances_[column_index][first_index][second_index] =
                distances_[column_index][second_index][first_index];
    }
}

void Split::CalculateAllDistances() {
    distances_ = std::vector<std::vector<std::vector<double>>>(
            num_columns_,
            std::vector<std::vector<double>>(num_rows_, std::vector<double>(num_rows_, 0)));
    min_max_dif_ = std::vector<model::DFConstraint>(num_columns_, {0, 0});
    type_ids_ = std::vector<model::TypeId>(num_columns_, model::TypeId::kUndefined);

    for (model::ColumnIndex column_index = 0; column_index < num_columns_; column_index++) {
        std::shared_ptr<model::PLI const> pli =
                relation_->GetColumnData(column_index).GetPliOwnership();
        std::deque<model::PLI::Cluster> const& index = pli->GetIndex();
        std::shared_ptr<std::vector<int> const> probing_table = pli->CalculateAndGetProbingTable();
        model::PLI::Cluster const& pt = *probing_table.get();

        double max_dif = 0, min_dif = std::numeric_limits<double>::max();
        for (std::size_t i = 0; i < index.size(); i++) {
            for (std::size_t j = 0; j < index.size(); j++) {
                std::size_t const first_index = index[i][0];
                std::size_t const second_index = index[j][0];
                if (first_index < num_rows_ && second_index < num_rows_) {
                    InsertDistance(column_index, first_index, second_index, min_dif, max_dif);
                }
            }
        }
        for (std::size_t i = 0; i < num_rows_; i++) {
            for (std::size_t j = 0; j < num_rows_; j++) {
                if (pt[i] != 0 && pt[j] != 0) {
                    distances_[column_index][i][j] =
                            distances_[column_index][index[pt[i] - 1][0]][index[pt[j] - 1][0]];
                    if (i != j && pt[i] == pt[j]) {
                        min_dif = 0;
                    }
                } else {
                    InsertDistance(column_index, i, j, min_dif, max_dif);
                }
            }
        }
        min_max_dif_[column_index] = {min_dif, max_dif};
    }
}

bool Split::IsFeasible(DF const& d) {
    for (std::size_t i = 0; i < num_rows_; i++) {
        for (std::size_t j = i + 1; j < num_rows_; j++) {
            if (CheckDF(d, {i, j})) return true;
        }
    }
    return false;
}

std::vector<DFConstraint> Split::IndexSearchSpace(model::ColumnIndex index) {
    std::vector<DFConstraint> dfs;

    if (!has_dif_table_) {
        // differential functions should be put in this exact order for further reducing
        for (int i = num_dfs_per_column_ - 1; i >= 0; i--) {
            if (min_max_dif_[index].IsWithinExclusive(i)) {
                dfs.emplace_back(min_max_dif_[index].lower_bound, i);
            }
        }
        return dfs;
    }

    std::size_t dif_num_rows = difference_typed_relation_->GetNumRows();

    model::TypedColumnData const& dif_column = difference_typed_relation_->GetColumnData(index);

    auto pair_compare = [](model::DFConstraint const& first_pair,
                           model::DFConstraint const& second_pair) {
        return first_pair.LongerThan(second_pair);
    };

    std::set<model::DFConstraint, decltype(pair_compare)> limits(pair_compare);

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
                            limits.insert(*intersect);
                        }
                    }
                }
            }
        }
    }

    // differential functions should be put in this exact order for further reducing
    for (auto limit : limits) {
        dfs.push_back(limit);
    }
    return dfs;
}

std::vector<DF> Split::SearchSpace(model::ColumnIndex index) {
    DF d = min_max_dif_;
    std::vector<DF> dfs;
    dfs.reserve(index_search_spaces_[index].size());
    dfs.push_back(d);
    for (auto const& df : index_search_spaces_[index]) {
        d[index] = df;
        dfs.push_back(d);
    }
    return dfs;
}

std::vector<DF> Split::SearchSpace(std::vector<model::ColumnIndex>& indices) {
    if (indices.size() == 1) return SearchSpace(indices.front());
    std::vector<DF> const last_column_search_space = SearchSpace(indices.back());
    indices.pop_back();
    std::vector<DF> const previous_columns_search_space = SearchSpace(indices);
    std::vector<DF> merged_search_space;
    DF intersect = min_max_dif_;
    for (auto const& first_df : previous_columns_search_space) {
        for (auto const& second_df : last_column_search_space) {
            for (model::ColumnIndex k = 0; k < num_columns_; k++) {
                intersect[k] = {std::max(first_df[k].lower_bound, second_df[k].lower_bound),
                                std::min(first_df[k].upper_bound, second_df[k].upper_bound)};
            }
            merged_search_space.push_back(intersect);
        }
    }
    return merged_search_space;
}

bool Split::Subsume(DF const& df1, DF const& df2) {
    for (model::ColumnIndex i = 0; i < num_columns_; i++) {
        if (!df2[i].IsSubsumedBy(df1[i])) {
            return false;
        }
    }
    return true;
}

std::vector<DF> Split::DoNegativePruning(std::vector<DF> const& search, DF const& last_df) {
    std::vector<DF> remainder;
    for (auto const& search_df : search) {
        if (search_df != last_df) {
            if (!Subsume(search_df, last_df)) {
                remainder.push_back(search_df);
            }
        }
    }
    return remainder;
}

std::pair<std::vector<DF>, std::vector<DF>> Split::NegativeSplit(std::vector<DF> const& search,
                                                                 DF const& last_df) {
    std::vector<DF> prune;
    std::vector<DF> remainder;
    for (auto const& search_df : search) {
        if (search_df != last_df) {
            if (Subsume(search_df, last_df)) {
                prune.push_back(search_df);
            } else {
                remainder.push_back(search_df);
            }
        }
    }
    return {std::move(prune), std::move(remainder)};
}

std::vector<DF> Split::DoPositivePruning(std::vector<DF> const& search, DF const& first_df) {
    std::vector<DF> remainder;
    for (auto const& search_df : search) {
        if (search_df != first_df) {
            if (!Subsume(first_df, search_df)) {
                remainder.push_back(search_df);
            }
        }
    }
    return remainder;
}

std::pair<std::vector<DF>, std::vector<DF>> Split::PositiveSplit(std::vector<DF> const& search,
                                                                 DF const& first_df) {
    std::vector<DF> prune;
    std::vector<DF> remainder;
    for (auto const& search_df : search) {
        if (search_df != first_df) {
            if (Subsume(first_df, search_df)) {
                prune.push_back(search_df);
            } else {
                remainder.push_back(search_df);
            }
        }
    }
    return {std::move(prune), std::move(remainder)};
}

// must be inline for optimization (gcc 11.4.0)
inline std::list<DD> Split::MergeReducedResults(std::list<DD> const& base_dds,
                                                std::list<DD> const& dds_to_merge) {
    std::list<DD> merged_dds;
    std::size_t const cur_size = base_dds.size();

    for (auto const& dd_to_merge : dds_to_merge) {
        bool include_dd = true;
        std::size_t index = 0;
        for (auto const& base_dd : base_dds) {
            if (Subsume(base_dd.lhs, dd_to_merge.lhs)) {
                include_dd = false;
                break;
            }
            index++;
            if (index == cur_size) break;
        }
        if (include_dd) merged_dds.push_back(dd_to_merge);
    }
    return merged_dds;
}

std::list<DD> Split::NegativePruningReduce(DF const& rhs, std::vector<DF> const& search,
                                           unsigned& cnt) {
    if (!search.size()) return {};

    DF const last_df = search.back();

    cnt++;
    if (!VerifyDD(last_df, rhs)) {
        std::vector<DF> remainder = DoNegativePruning(search, last_df);
        return NegativePruningReduce(rhs, remainder, cnt);
    }

    auto const [prune, remainder] = NegativeSplit(search, last_df);

    std::list<DD> dds = NegativePruningReduce(rhs, prune, cnt);
    if (!dds.size() && IsFeasible(last_df)) dds.emplace_back(last_df, rhs);
    std::list<DD> const remaining_dds = NegativePruningReduce(rhs, remainder, cnt);

    std::list<DD> merged_dds = MergeReducedResults(dds, remaining_dds);
    dds.splice(dds.end(), merged_dds);

    return dds;
}

std::list<DD> Split::HybridPruningReduce(DF const& rhs, std::vector<DF> const& search,
                                         unsigned& cnt) {
    if (!search.size()) return {};

    std::list<DD> dds;
    DF const first_df = search.front();
    DF const last_df = search.back();

    cnt++;
    if (VerifyDD(first_df, rhs)) {
        if (IsFeasible(first_df)) dds.emplace_back(first_df, rhs);
        std::vector<DF> remainder = DoPositivePruning(search, first_df);
        std::list<DD> remaining_dds = HybridPruningReduce(rhs, remainder, cnt);
        dds.splice(dds.end(), remaining_dds);
        return dds;
    }

    cnt++;
    if (!VerifyDD(last_df, rhs)) {
        std::vector<DF> remainder = DoNegativePruning(search, last_df);
        return HybridPruningReduce(rhs, remainder, cnt);
    }

    auto const [prune, remainder] = PositiveSplit(search, first_df);

    dds = HybridPruningReduce(rhs, remainder, cnt);
    std::list<DD> const pruning_dds = HybridPruningReduce(rhs, prune, cnt);

    std::list<DD> merged_dds = MergeReducedResults(dds, pruning_dds);
    dds.splice(dds.end(), merged_dds);

    return dds;
}

std::list<DD> Split::InstanceExclusionReduce(
        std::vector<std::pair<std::size_t, std::size_t>> const& tuple_pairs,
        std::vector<DF> const& search, DF const& rhs, unsigned& cnt) {
    if (!search.size()) return {};

    std::list<DD> dds;
    DF const first_df = search.front();
    DF const last_df = search.back();
    std::vector<std::pair<std::size_t, std::size_t>> remaining_tuple_pairs;

    cnt++;
    bool last_dd_holds = true;
    bool no_pairs_left = true;
    for (auto pair : tuple_pairs) {
        if (!CheckDF(rhs, pair)) {
            if (CheckDF(first_df, pair)) {
                remaining_tuple_pairs.push_back(pair);
                no_pairs_left = false;
            }
            if (last_dd_holds && CheckDF(last_df, pair)) last_dd_holds = false;
            if (!no_pairs_left && !last_dd_holds) break;
        }
    }

    if (no_pairs_left) {
        if (IsFeasible(first_df)) dds.emplace_back(first_df, rhs);
        std::vector<DF> remainder = DoPositivePruning(search, first_df);
        std::list<DD> remaining_dds = InstanceExclusionReduce(tuple_pairs, remainder, rhs, cnt);
        dds.splice(dds.end(), remaining_dds);
        return dds;
    }

    cnt++;

    if (!last_dd_holds) {
        std::vector<DF> remainder = DoNegativePruning(search, last_df);
        return InstanceExclusionReduce(tuple_pairs, remainder, rhs, cnt);
    }

    auto const [prune, remainder] = PositiveSplit(search, first_df);

    dds = InstanceExclusionReduce(tuple_pairs, remainder, rhs, cnt);
    std::list<DD> const pruning_dds =
            InstanceExclusionReduce(remaining_tuple_pairs, prune, rhs, cnt);

    std::list<DD> merged_dds = MergeReducedResults(dds, pruning_dds);
    dds.splice(dds.end(), merged_dds);

    return dds;
}

void Split::CalculateTuplePairs() {
    for (std::size_t i = 0; i < num_rows_; i++) {
        for (std::size_t j = i + 1; j < num_rows_; j++) {
            tuple_pairs_.push_back({i, j});
        }
    }
}

void Split::PrintResults() {
    std::list<model::DDString> const result_strings = GetDDStringList();
    LOG(INFO) << "Minimal cover size: " << result_strings.size();
    for (auto const& result_str : result_strings) {
        LOG(DEBUG) << result_str.ToString();
    }
}

std::list<DD> const& Split::GetDDs() const {
    return dd_collection_;
}

std::vector<model::DFConstraint> const& Split::GetMinMaxDif() const {
    return min_max_dif_;
}

model::DDString Split::DDToDDString(DD const& dd) const {
    model::DDString dd_string;
    for (model::ColumnIndex index = 0; index < num_columns_; index++) {
        if (dd.lhs[index] != min_max_dif_[index]) {
            dd_string.left.emplace_back(input_table_->GetColumnName(non_empty_cols_[index]),
                                        dd.lhs[index]);
        }
    }

    for (model::ColumnIndex index = 0; index < num_columns_; index++) {
        if (dd.rhs[index] != min_max_dif_[index]) {
            dd_string.right.emplace_back(input_table_->GetColumnName(non_empty_cols_[index]),
                                         dd.rhs[index]);
        }
    }
    return dd_string;
}

std::list<model::DDString> Split::GetDDStringList() const {
    std::list<model::DDString> dd_strings;
    for (auto const& result_dd : dd_collection_) {
        dd_strings.emplace_back(DDToDDString(result_dd));
    }
    return dd_strings;
}

}  // namespace algos::dd
