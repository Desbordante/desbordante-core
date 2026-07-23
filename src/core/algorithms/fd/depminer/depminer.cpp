#include "core/algorithms/fd/depminer/depminer.h"

#include <list>
#include <memory>

#include "core/algorithms/fd/make_lhslim_lhs_mask_adder.h"
#include "core/config/max_lhs/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/model/table/compute_agree_sets.h"
#include "core/model/table/create_stripped_partitions.h"
#include "core/util/bitset_utils.h"
#include "core/util/logger.h"

namespace algos::fd {

using boost::dynamic_bitset, std::make_shared, std::shared_ptr, std::setw, std::vector, std::list,
        std::dynamic_pointer_cast;

Depminer::Depminer() {
    RegisterOptions();

    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void Depminer::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
}

void Depminer::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kMaxLhsOpt.GetName()});
}

void Depminer::ResetState() {
    fd_view_ = nullptr;
}

void Depminer::LoadDataInternal() {
    std::vector<model::PositionListIndex> plis = model::CreateStrippedPartitions(*input_table_);
    if (plis.empty() || plis.front().GetRelationSize() == 0) {
        throw std::runtime_error("Got an empty dataset: FD mining is meaningless.");
    }
    plis_ = {std::move(plis)};

    table_header_ = model::TableHeader::FromDatasetStream(*input_table_);
}

void Depminer::ExecuteInternal() {
    // Agree sets
    ColumnCombinations const agree_sets = model::ComputeAgreeSets(plis_);

    // maximal sets
    std::vector<ColumnCombinations> const c_max_sets = GenerateCmaxSets(agree_sets);

    std::size_t const column_num = plis_.GetStrippedPartitions().size();
    LhsMaskFdView::Storage lhs_masks(column_num);
    auto report_fd = MakeLhsLimLhsMaskAdder(lhs_masks, max_lhs_);
    // LHS
    for (model::Index column_index = 0; column_index != column_num; ++column_index) {
        LhsForColumn(column_index, report_fd, c_max_sets[column_index]);
    }

    fd_view_ = std::make_shared<LhsMaskFdView>(table_header_, std::move(lhs_masks));
}

auto Depminer::GenerateCmaxSets(ColumnCombinations const& agree_sets)
        -> std::vector<ColumnCombinations> {
    std::size_t const column_number = table_header_.column_names.size();
    std::vector<ColumnCombinations> c_max_sets;
    c_max_sets.reserve(column_number);

    for (model::Index column_index = 0; column_index != column_number; ++column_index) {
        // finding max sets
        ColumnCombinations maximal_sets;

        for (boost::dynamic_bitset<> const& agree_set : agree_sets) {
            // A maximal set is an attribute set X which, for some attribute A, is the largest
            // possible set not determining A.
            if (agree_set.test(column_index)) continue;
            // There is a pair where the values at the current column are not equal.

            bool is_maximal = true;
            for (auto it = maximal_sets.begin(); it != maximal_sets.end();) {
                boost::dynamic_bitset<> const& super_set_candidate = *it;
                if (is_maximal && agree_set.is_subset_of(super_set_candidate)) {
                    is_maximal = false;
                }
                if (super_set_candidate.is_subset_of(agree_set)) {
                    maximal_sets.erase(it++);
                } else {
                    ++it;
                }
            }
            if (is_maximal) {
                maximal_sets.insert(agree_set);
            }
        }

        // "The collection cmax(dep(r), A) of complements of maximal sets max(dep(r), A) is a simple
        // hypergraph."
        ColumnCombinations& max_set_complements = c_max_sets.emplace_back();
        for (auto it = maximal_sets.begin(); it != maximal_sets.end();) {
            ColumnCombinations::node_type node = maximal_sets.extract(it++);
            node.value().flip();
            // "A collection H of subsets of R is a simple hypergraph if \forall X \in H, X !=
            // \emptyset and ..."
            assert(node.value().any());
            max_set_complements.insert(std::move(node));
        }
    }

    return c_max_sets;
}

void Depminer::LhsForColumn(model::Index column, BitsetAndIndexResultReporter const& report_fd,
                            ColumnCombinations const& c_max_set) {
    auto const& pli = plis_.GetStrippedPartition(column);
    bool column_contains_only_equal_values = pli.IsConstant();
    if (column_contains_only_equal_values) {
        report_fd(boost::dynamic_bitset<>(table_header_.column_names.size()), column);
        return;
    }

    ColumnCombinations level = GenFirstLevel(c_max_set);

    while (!level.empty()) {
        for (auto it = level.begin(); it != level.end();) {
            boost::dynamic_bitset<> const& cur_level_column_combination = *it;
            bool is_fd = true;
            for (auto const& combination : c_max_set) {
                if (!cur_level_column_combination.intersects(combination)) {
                    is_fd = false;
                    break;
                }
            }
            if (is_fd) {
                if (!cur_level_column_combination.test(column)) {
                    report_fd(cur_level_column_combination, column);
                }
                it = level.erase(it);
            } else {
                ++it;
            }
        }
        level = GenNextLevel(level);
    }
}

auto Depminer::GenFirstLevel(ColumnCombinations const& cmax_set) -> ColumnCombinations {
    if (cmax_set.empty()) return {};
    ColumnCombinations level;
    // The name should be something like ???_columns, but I don't get what this is doing for now.
    boost::dynamic_bitset<> set_bits(cmax_set.begin()->size());
    for (boost::dynamic_bitset<> const& combination : cmax_set) {
        set_bits |= combination;
    }
    boost::dynamic_bitset<> single_column_scratch(cmax_set.begin()->size());
    util::ForEachIndex(set_bits, [&](model::Index i) {
        single_column_scratch.set(i);
        level.insert(single_column_scratch);
        single_column_scratch.reset(i);
    });
    return level;
}

// Apriori-gen function
auto Depminer::GenNextLevel(ColumnCombinations const& prev_level) -> ColumnCombinations {
    ColumnCombinations candidates;
    for (auto const& p : prev_level) {
        for (auto const& q : prev_level) {
            if (!CheckJoin(p, q) /* Tries to avoid some extra additions? */) {
                continue;
            }
            candidates.insert(p | q);
        }
    }

    ColumnCombinations next_level;
    for (auto it = candidates.begin(); it != candidates.end();) {
        auto node = candidates.extract(it++);
        boost::dynamic_bitset<>& candidate = node.value();
        bool keep = true;
        for (auto index = candidate.find_first(); index != boost::dynamic_bitset<>::npos;
             index = candidate.find_next(index)) {
            candidate.reset(index);
            if (!prev_level.contains(candidate)) {
                keep = false;
                break;
            }
            candidate.set(index);
        }
        if (keep) {
            next_level.insert(std::move(candidate));
        }
    }
    return next_level;
}

bool Depminer::CheckJoin(boost::dynamic_bitset<> const& p, boost::dynamic_bitset<> const& q) {
    size_t p_last = -1, q_last = -1;

    for (size_t i = 0; i < p.size(); i++) {
        p_last = p[i] ? i : p_last;
        q_last = q[i] ? i : q_last;
    }
    if (p_last >= q_last) return false;
    /* The above part avoids processing the same pair in a different order? But [1 2 5], [2 3 5] are
     * not let through? */
    // This works because the previous level will hold a different decomposition of [1 2 3 5], since
    // all the new element's subsets of len - 1 have to be in the previous level.
    // What? This reduces to p.count() == q.count(), which always holds.
    dynamic_bitset<> intersection = p;
    intersection.intersects(q);
    return p.count() == intersection.count() && q.count() == intersection.count();
}

}  // namespace algos::fd
