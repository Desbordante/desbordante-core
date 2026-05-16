#pragma once

#include <algorithm>
#include <cstddef>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/model/differential_dependency.h"
#include "core/algorithms/dd/fastdd/model/match_df.h"
#include "core/algorithms/dd/fastdd/trees/translating_minimize_tree.h"
#include "core/algorithms/dd/fastdd/trees/tree_search.h"
#include "core/algorithms/dd/fastdd/util/bitset_concept.h"
#include "core/algorithms/dd/fastdd/util/dif_func_info.h"
#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"
#include "core/algorithms/dd/fastdd/util/evidence_inverter.h"
#include "core/algorithms/dd/fastdd/util/hitting_set_enumeration_strategy.h"
#include "core/algorithms/dd/fastdd/util/mmcs.h"
#include "core/util/logger.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
class HybridEvidenceInverter {
private:
    std::vector<Bitset> match_dfs_;
    std::vector<std::vector<DifferentialFunction>> dif_funcs_;
    std::vector<Bitset> column_to_dif_funcs_;
    std::shared_ptr<DifFuncInfo<Bitset> const> dif_func_info_;

    std::vector<boost::dynamic_bitset<>> dif_func_to_satisfied_bitsets_;
    std::vector<boost::dynamic_bitset<>> dif_func_to_not_satisfied_bitsets_;

    std::unordered_map<std::size_t, std::shared_ptr<TranslatingMinimizeTree<Bitset>>>
            minimize_tree_map_;

    static constexpr HittingSetEnumerationStrategy strategy_ = HittingSetEnumerationStrategy::MMCS;

    void BuildClueIndices() {
        dif_func_to_satisfied_bitsets_.reserve(dif_func_info_->dif_func_num_);
        dif_func_to_not_satisfied_bitsets_.reserve(dif_func_info_->dif_func_num_);

        for (std::size_t i = 0; i != dif_func_info_->dif_func_num_; ++i) {
            dif_func_to_satisfied_bitsets_.emplace_back(match_dfs_.size());
            dif_func_to_not_satisfied_bitsets_.emplace_back(match_dfs_.size());
        }
        for (std::size_t i = 0; i != match_dfs_.size(); ++i) {
            Bitset const& diff_bitset = match_dfs_[i];
            for (std::size_t j = 0; j != dif_func_info_->dif_func_num_; ++j) {
                if (diff_bitset[j]) {
                    dif_func_to_satisfied_bitsets_[j].set(i);
                } else {
                    dif_func_to_not_satisfied_bitsets_[j].set(i);
                }
            }
        }
    }

    std::vector<Bitset> MinimizeDifferentialSet(std::vector<Bitset> bitsets) const {
        LOG_TRACE("Start minimize");
        std::ranges::sort(bitsets,
                          [](Bitset const& a, Bitset const& b) { return a.count() > b.count(); });
        LOG_TRACE("Sorted");

        TreeSearch<Bitset> negative_search;
        LOG_TRACE("Built negative search: {}", bitsets.size());
        std::ranges::for_each(bitsets, [&negative_search](Bitset const& bitset) {
            if (!negative_search.FindSuperSet(bitset)) {
                negative_search.Add(bitset);
            }
        });
        LOG_TRACE("Iterate");
        return std::vector<Bitset>(negative_search.begin(), negative_search.end());
    }

    std::vector<std::vector<DifferentialFunction>> Minimize(std::vector<Bitset> covers,
                                                            std::vector<Bitset> cur_match_dfs,
                                                            std::size_t rhs_column,
                                                            std::size_t rhs_offset) {
        std::shared_ptr<TranslatingMinimizeTree<Bitset>> minimize_tree;
        std::size_t const minimize_tree_key =
                dif_func_info_
                        ->dif_func_to_column_index_[dif_func_info_->dif_func_nums_[rhs_column] +
                                                    rhs_offset];
        if (minimize_tree_map_[minimize_tree_key]) {
            minimize_tree = minimize_tree_map_[minimize_tree_key];
        } else {
            minimize_tree = std::make_shared<TranslatingMinimizeTree<Bitset>>(dif_func_info_);
            minimize_tree_map_[minimize_tree_key] = minimize_tree;
        }

        std::vector<Bitset> minimized_bitsets = minimize_tree->Minimize(std::move(covers));

        std::vector<std::vector<DifferentialFunction>> minimized_lhs;
        minimized_lhs.reserve(minimized_bitsets.size());
        for (auto const& bitset : minimized_bitsets) {
            // check if LHS is not trivial
            if (std::ranges::any_of(cur_match_dfs, [&bitset](auto const& match_df) {
                    return bitset.is_subset_of(match_df);
                })) {
                std::vector<DifferentialFunction> lhs;
                lhs.reserve(bitset.count());
                for (std::size_t index = bitset.find_first(); index != Bitset::npos;
                     index = bitset.find_next(index)) {
                    std::size_t const column_index =
                            dif_func_info_->dif_func_to_column_index_[index];
                    std::size_t const df_offset = dif_func_info_->dif_func_to_offset_[index];
                    lhs.push_back(dif_funcs_[column_index][df_offset]);
                }

                minimized_lhs.push_back(std::move(lhs));
            }
        }

        return minimized_lhs;
    }

    // TODO: extend the method so that it can remove longer transitive paths (something like
    // transitive reduction in graph theory). However it is unclear, whether removing all transitive
    // DDs is necessary (it is not done in other primitives).
    std::vector<DifferentialDependency> RemoveTransitive(
            std::vector<std::vector<std::vector<DifferentialFunction>>> dds) const {
        auto subsume = [](std::vector<DifferentialFunction> const& lhs,
                          DifferentialFunction const& rhs) {
            return lhs.size() == 1 && rhs.GetColumn() == lhs.front().GetColumn() &&
                   rhs.GetConstraint().IsSubsumedBy(lhs.front().GetConstraint());
        };

        std::vector<DifferentialDependency> result;
        std::size_t index = 0;
        for (std::size_t rhs_column = 0; rhs_column != dif_funcs_.size(); ++rhs_column) {
            for (std::size_t rhs_offset = dif_funcs_[rhs_column].size(); rhs_offset != 0;
                 --rhs_offset) {
                while (true) {
                    bool is_removable = false;
                    std::vector<std::vector<DifferentialFunction>> size_one_lhs;
                    for (auto const& lhs : dds[index]) {
                        if (lhs.size() == 1) {
                            size_one_lhs.push_back(lhs);
                        }
                    }
                    std::vector<std::vector<DifferentialFunction>> new_dds_index;
                    new_dds_index.reserve(dds[index].size());

                    for (auto const& first_lhs : dds[index]) {
                        bool remove = false;
                        if (!is_removable) {
                            for (auto const& second_lhs : size_one_lhs) {
                                std::size_t second_index = 0;
                                for (std::size_t second_rhs_column = 0;
                                     second_rhs_column != dif_funcs_.size(); ++second_rhs_column) {
                                    for (std::size_t second_rhs_offset =
                                                 dif_funcs_[second_rhs_column].size();
                                         second_rhs_offset != 0; --second_rhs_offset) {
                                        if (subsume(second_lhs,
                                                    dif_funcs_[second_rhs_column]
                                                              [second_rhs_offset - 1])) {
                                            if (std::ranges::find(dds[second_index], first_lhs) !=
                                                dds[second_index].end()) {
                                                if (!is_removable) remove = true;
                                                is_removable = true;
                                                break;
                                            }
                                        }
                                        ++second_index;
                                    }
                                    if (remove) {
                                        break;
                                    }
                                }
                                if (remove) {
                                    break;
                                }
                            }
                        }
                        if (!remove) {
                            new_dds_index.push_back(first_lhs);
                        }
                    }

                    if (!is_removable) {
                        break;
                    }

                    dds[index] = std::move(new_dds_index);
                }

                DifferentialFunction rhs = dif_funcs_[rhs_column][rhs_offset - 1];
                for (auto const& lhs : dds[index]) {
                    result.emplace_back(lhs, rhs);
                }
                ++index;
            }
        }

        return result;
    }

public:
    HybridEvidenceInverter(std::vector<Bitset> match_dfs,
                           DifferentialFunctionBuilder const& df_builder)
        : match_dfs_(std::move(match_dfs)) {
        dif_funcs_ = df_builder.GetDifFuncs();

        std::vector<std::size_t> dif_func_sizes;
        std::vector<std::size_t> dif_func_nums;
        std::vector<std::size_t> dif_func_to_column_index;
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
        dif_func_to_column_index.reserve(dif_func_num);
        dif_func_to_offset.reserve(dif_func_num);
        column_to_dif_funcs_.reserve(dif_funcs_.size());
        for (std::size_t i = 0; i != dif_funcs_.size(); ++i) {
            Bitset cur_column_bitset(dif_func_num);
            for (std::size_t j = 0; j != dif_funcs_[i].size(); ++j) {
                dif_func_to_column_index.push_back(i);
                cur_column_bitset.set(dif_func_nums[i] + j);
                dif_func_to_offset.push_back(j);
            }
            column_to_dif_funcs_.push_back(std::move(cur_column_bitset));
        }

        std::vector<Bitset> dif_func_to_bitset(dif_func_num, Bitset(dif_func_num));
        for (auto& bitset : dif_func_to_bitset) {
            bitset.flip();
        }
        for (std::size_t i = 0; i != dif_funcs_.size(); ++i) {
            std::size_t const cur_index = dif_func_nums[i];
            std::vector<ThresholdInfo> const& thresholds = df_builder.GetThresholds(i);
            std::list<ThresholdInfo> threshold_list;
            for (auto const& threshold : thresholds) {
                if (!threshold.is_upper_bound) {
                    threshold_list.push_back(threshold);
                } else {
                    auto threshold_it = threshold_list.begin();
                    for (; threshold_it != threshold_list.end(); ++threshold_it) {
                        dif_func_to_bitset[cur_index + threshold.dif_func_index].set(
                                cur_index + threshold_it->dif_func_index, false);
                        if (threshold_it->dif_func_index == threshold.dif_func_index) {
                            break;
                        }
                    }
                    threshold_list.erase(threshold_it);
                }
            }
        }

        dif_func_info_ = std::make_shared<DifFuncInfo<Bitset> const>(
                std::move(dif_func_sizes), std::move(dif_func_nums),
                std::move(dif_func_to_column_index), std::move(dif_func_to_offset),
                std::move(dif_func_to_bitset), dif_func_num, dif_funcs_.size());
    }

    std::vector<DifferentialDependency> BuildDDs() {
        BuildClueIndices();
        LOG_DEBUG("Built clue indices");

        std::vector<std::vector<std::vector<DifferentialFunction>>> cur_result;
        cur_result.reserve(dif_func_info_->dif_func_num_);
        std::size_t cur_result_size = 0;

        for (std::size_t i = 0; i != dif_funcs_.size(); ++i) {
            for (std::size_t j = dif_funcs_[i].size(); j != 0; --j) {
                std::size_t const dif_func_index = dif_func_info_->dif_func_nums_[i] + j - 1;
                boost::dynamic_bitset<> not_satisfied_bitset =
                        dif_func_to_not_satisfied_bitsets_[dif_func_index];
                boost::dynamic_bitset<> satisfied_bitset =
                        dif_func_to_satisfied_bitsets_[dif_func_index];
                if (not_satisfied_bitset.none() || satisfied_bitset.none()) {
                    continue;
                }
                std::vector<Bitset> not_satisfied_diff_bitsets;
                not_satisfied_diff_bitsets.reserve(not_satisfied_bitset.count());
                std::vector<Bitset> satisfied_diff_bitsets;
                satisfied_diff_bitsets.reserve(satisfied_bitset.count());
                for (std::size_t index = 0; index != match_dfs_.size(); ++index) {
                    Bitset diff_bitset = match_dfs_[index];
                    for (std::size_t k = dif_func_info_->dif_func_nums_[i];
                         k != dif_func_info_->dif_func_nums_[i + 1]; ++k) {
                        diff_bitset.set(k, false);
                    }
                    if (not_satisfied_bitset[index]) {
                        not_satisfied_diff_bitsets.push_back(std::move(diff_bitset));
                    } else {
                        satisfied_diff_bitsets.push_back(std::move(diff_bitset));
                    }
                }
                LOG_DEBUG("Col {}; Dif_func {}; {}", i, j - 1, dif_funcs_[i][j - 1].ToString());
                not_satisfied_diff_bitsets =
                        MinimizeDifferentialSet(std::move(not_satisfied_diff_bitsets));
                satisfied_diff_bitsets = MinimizeDifferentialSet(std::move(satisfied_diff_bitsets));

                std::vector<Bitset> covers;
                if constexpr (strategy_ == HittingSetEnumerationStrategy::EvidenceInverter) {
                    EvidenceInverter<Bitset> inverter(std::move(not_satisfied_diff_bitsets),
                                                      dif_func_info_->dif_func_num_,
                                                      column_to_dif_funcs_, i);
                    LOG_DEBUG("Built inverter");
                    covers = inverter.GetCovers();
                } else {
                    MMCS<Bitset> mmcs(std::move(not_satisfied_diff_bitsets), column_to_dif_funcs_,
                                      i);
                    LOG_DEBUG("Built MMCS");
                    covers = mmcs.GetCovers();
                }
                LOG_DEBUG("Got covers: {}", covers.size());

                std::vector<std::vector<DifferentialFunction>> minimized_covers =
                        Minimize(std::move(covers), std::move(satisfied_diff_bitsets), i, j - 1);
                LOG_DEBUG("Minimized covers: {}", minimized_covers.size());
                cur_result_size += minimized_covers.size();
                cur_result.push_back(std::move(minimized_covers));
            }
        }
        LOG_INFO("Built and minimized covers: {}", cur_result_size);

        std::vector<DifferentialDependency> result = RemoveTransitive(std::move(cur_result));
        LOG_INFO("Removed transitive DDs");

        return result;
    }
};

}  // namespace algos::dd
