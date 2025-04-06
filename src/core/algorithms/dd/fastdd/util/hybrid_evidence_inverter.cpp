#include "core/algorithms/dd/fastdd/util/hybrid_evidence_inverter.h"

#include <unordered_set>
#include <utility>

#include "core/algorithms/dd/fastdd/trees/translating_minimize_tree.h"
#include "core/algorithms/dd/fastdd/util/evidence_inverter.h"
#include "core/util/logger.h"

namespace algos::dd {

HybridEvidenceInverter::HybridEvidenceInverter(std::vector<MatchDF> match_dfs,
                                               DifferentialFunctionBuilder const& df_builder)
    : match_dfs_(std::move(match_dfs)) {
    dif_funcs_ = df_builder.GetDifFuncs();

    std::vector<std::size_t> dif_func_sizes;
    std::vector<std::size_t> dif_func_nums;
    std::vector<std::size_t> dif_func_to_node_id;
    std::vector<std::size_t> dif_func_to_offset;
    std::size_t dif_func_num;

    dif_func_sizes.reserve(dif_funcs_.size());
    dif_func_nums.reserve(dif_funcs_.size() + 1);
    dif_func_nums.push_back(0);
    for (std::size_t i = 0; i != dif_funcs_.size(); ++i) {
        dif_func_sizes.push_back(dif_funcs_[i].size());
        dif_func_nums.push_back(dif_func_nums[i] + dif_func_sizes[i]);
    }
    dif_func_num = dif_func_nums[dif_func_nums.size() - 1];
    dif_func_to_node_id.reserve(dif_func_num);
    dif_func_to_offset.reserve(dif_func_num);
    column_to_dif_funcs_.reserve(dif_funcs_.size());
    for (std::size_t i = 0; i != dif_funcs_.size(); ++i) {
        boost::dynamic_bitset<> cur_column_bitset(dif_func_num);
        for (std::size_t j = 0; j != dif_funcs_[i].size(); ++j) {
            if (dif_funcs_[i][j].GetOperator() == Operator::kLessOrEqual) {
                dif_func_to_node_id.push_back(i);
            } else {
                dif_func_to_node_id.push_back(i + dif_funcs_.size());
            }
            cur_column_bitset.set(dif_func_nums[i] + j);
            dif_func_to_offset.push_back(j);
        }
        column_to_dif_funcs_.push_back(std::move(cur_column_bitset));
    }

    dif_func_info_ = std::make_shared<DifFuncInfo const>(
            std::move(dif_func_sizes), std::move(dif_func_nums), std::move(dif_func_to_node_id),
            std::move(dif_func_to_offset), dif_func_num, dif_funcs_.size());
}

void HybridEvidenceInverter::BuildClueIndices() {
    dif_func_to_satisfied_bitsets_.reserve(dif_func_info_->dif_func_num_);
    dif_func_to_not_satisfied_bitsets_.reserve(dif_func_info_->dif_func_num_);

    for (std::size_t i = 0; i != dif_func_info_->dif_func_num_; ++i) {
        dif_func_to_satisfied_bitsets_.emplace_back(match_dfs_.size());
        dif_func_to_not_satisfied_bitsets_.emplace_back(match_dfs_.size());
    }
    for (std::size_t i = 0; i != match_dfs_.size(); ++i) {
        boost::dynamic_bitset<> diff_bitset = match_dfs_[i].GetBitset();
        for (std::size_t j = 0; j != dif_func_info_->dif_func_num_; ++j) {
            if (diff_bitset[j]) {
                dif_func_to_satisfied_bitsets_[j].set(i);
            } else {
                dif_func_to_not_satisfied_bitsets_[j].set(i);
            }
        }
    }
}

std::vector<DifferentialDependency> HybridEvidenceInverter::BuildDDs() {
    BuildClueIndices();
    LOG_DEBUG("Built clue indices");

    std::vector<DifferentialDependency> result;

    for (std::size_t i = 0; i != dif_funcs_.size(); ++i) {
        for (std::size_t j = dif_funcs_[i].size(); j != 0; --j) {
            boost::dynamic_bitset<> cur_bitset =
                    dif_func_to_not_satisfied_bitsets_[dif_func_info_->dif_func_nums_[i] + j - 1];
            if (cur_bitset.none()) {
                continue;
            }
            std::vector<boost::dynamic_bitset<>> cur_diff_bitsets;
            for (std::size_t index = cur_bitset.find_first();
                 index != boost::dynamic_bitset<>::npos; index = cur_bitset.find_next(index)) {
                boost::dynamic_bitset<> diff_bitset = match_dfs_[index].GetBitset();
                for (std::size_t k = dif_func_info_->dif_func_nums_[i];
                     k != dif_func_info_->dif_func_nums_[i + 1]; ++k) {
                    diff_bitset.set(k, false);
                }
                cur_diff_bitsets.push_back(std::move(diff_bitset));
            }
            LOG_DEBUG("Col {}; Dif_func {}; {}", i, j - 1, dif_funcs_[i][j - 1].ToString());

            EvidenceInverter inverter(std::move(cur_diff_bitsets), dif_func_info_->dif_func_num_,
                                      column_to_dif_funcs_, i);
            LOG_DEBUG("Built inverter");
            std::unordered_set<boost::dynamic_bitset<>> covers_set = inverter.GetCovers();
            std::vector<boost::dynamic_bitset<>> covers(covers_set.begin(), covers_set.end());
            LOG_DEBUG("Got covers: {}", covers.size());
            std::vector<DifferentialDependency> minimized_covers =
                    Minimize(std::move(covers), i, j - 1);
            LOG_DEBUG("Minimized covers: {}", minimized_covers.size());
            std::move(minimized_covers.begin(), minimized_covers.end(), std::back_inserter(result));
        }
    }

    return result;
}

std::vector<DifferentialDependency> HybridEvidenceInverter::Minimize(
        std::vector<boost::dynamic_bitset<>> covers, std::size_t rhs_column,
        std::size_t rhs_offset) {
    std::shared_ptr<TranslatingMinimizeTree> minimize_tree;
    std::size_t const minimize_tree_key =
            dif_func_info_
                    ->dif_func_to_node_id_[dif_func_info_->dif_func_nums_[rhs_column] + rhs_offset];
    if (minimize_tree_map_[minimize_tree_key]) {
        minimize_tree = minimize_tree_map_[minimize_tree_key];
    } else {
        minimize_tree = std::make_shared<TranslatingMinimizeTree>(dif_func_info_);
        minimize_tree_map_[minimize_tree_key] = minimize_tree;
    }
    std::unordered_set<boost::dynamic_bitset<>> minimized_bitsets =
            minimize_tree->Minimize(std::move(covers));

    std::vector<DifferentialDependency> minimized_dds;
    minimized_dds.reserve(minimized_bitsets.size());
    for (auto const& bitset : minimized_bitsets) {
        std::vector<DifferentialFunction> lhs;
        lhs.reserve(bitset.count());
        for (std::size_t index = bitset.find_first(); index != boost::dynamic_bitset<>::npos;
             index = bitset.find_next(index)) {
            std::size_t const node_id = dif_func_info_->dif_func_to_node_id_[index];
            std::size_t const column_index = node_id % dif_funcs_.size();
            std::size_t const df_offset = dif_func_info_->dif_func_to_offset_[index];
            lhs.push_back(dif_funcs_[column_index][df_offset]);
        }

        DifferentialFunction rhs = dif_funcs_[rhs_column][rhs_offset];
        minimized_dds.emplace_back(std::move(lhs), std::move(rhs));
        LOG_TRACE("{}", minimized_dds[minimized_dds.size() - 1].ToString());
    }

    return minimized_dds;
}

}  // namespace algos::dd
