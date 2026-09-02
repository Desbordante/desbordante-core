#include "core/algorithms/fd/tane/tane.h"

#include <algorithm>
#include <memory>
#include <ranges>

#include "core/algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "core/config/error/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/model/table/column_data.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/util/bitset_utils.h"
#include "core/util/logger.h"

namespace algos {
// The original TANE algorithm is incorrect, because it deletes superkey nodes too eagerly, leading
// to failures when the intersection on line 6 of PRUNE is checked, as we might not have one or more
// of the intersected C^+ sets calculated.
// This shows itself when there is a column in C^+(X) \ X of a key (line 5 of PRUNE) that belongs to
// another key of smaller size, as said key would be deleted when processing a previous level, with
// line 2 in COMPUTE_DEPENDENCIES consequently not calculating the further C^+ sets.
// One solution is to directly check each X \ {B} -> A (B ∈ X, A ∉ X) dependency when encountering a
// key if no other C^+(X ∪ {A} \ {B}) that doesn't contain A exists (i. e. no evidence if
// X \ {B} -> A holds). If we do happen upon it, we know that X \ {B} -> A holds by definition of
// C^+(X) and don't have to check that directly. There would be a PLI available for each X \ {B}, as
// those had to have been part of the previous level for X to end up in the current one.
// Another is to, instead of deleting the key nodes, keep them and mark them, then skip the marked
// ones in line 3 of COMPUTE_DEPENDENCIES, but not in lines 1 and 2 (i.e. keep calculating their
// C^+(X)). This is the solution in the Metanome implementation.

// The e(X) − e(X ∪ {A}) ≤ e(X → A) ≤ e(X) error bound optimization is omitted to make the algorithm
// easier to generalize to other measures: figuring out how to define them for keys would take some
// effort, and it is not guaranteed that the bounds would hold up under the most sensible
// generalization.

// What if we avoid performing a part of PRUNE?
// 1. Don't delete empty, don't remove keys.
// - No pruning, but everything works.
// 2. Don't delete empty, remove keys.
// - I can't think of a way to use this beneficially.
// 3. Delete empty, don't remove keys.
// - We can assume C^+ is empty if we don't find it, so it makes no sense to implement 1, other than
//   checking what effect the pruning has.
// 4. Delete empty, remove keys.
// - We have to use a direct FD check if we don't find C^+, as it may have been due to a removed key
//   instead of it being empty.
// 4.1. Remove keys immediately.
// 4.2. Keep keys, remove on another pass.
// - Avoids some direct FD checks.

// TODO: Implement 3, 4.1, 4.2.
// When implementing 3, we should use a single dict where the value contains the PLI, given that
// PLIs and RHS candidates are added together. When finding a key in Prune, the PLI can be set to
// nullptr.
// In 4.1 deleting this combined values would delete the PLI for a sibling key, which we'll have to
// calculate again if we don't see a set that does not contain the column in line 5 of PRUNE. To
// mitigate, we can specifically store the key's column combination bitset, then delete the stored
// ones, which is 4.2.

// However, we can also keep PLIs in a separate map. That way, we can avoid recalculating any PLIs
// in that case. This is a middle ground between 4.1 and 4.2. We can also, even with separate maps,
// remove the PLIs on a second pass instead of immediately, like 4.2 (needs storing the key column
// combinations). However, I doubt that would do much, keys are not that common.

using boost::dynamic_bitset;
using Cluster = model::PositionListIndex::Cluster;

Tane::Tane() : PliBasedAFDAlgorithm() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::kErrorOpt(&max_fd_error_));
    RegisterOption(Option{&afd_measure_, kAfdMeasure, kDAfdMeasure, model::AfdMeasure::kG1});
}

void Tane::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::kErrorOpt.GetName(), config::names::kAfdMeasure});
}

bool Tane::IsKey(model::PositionListIndex const* pli) {
    return pli->AllValuesAreUnique();
}

void Tane::Prune(LevelColumnCombinationsInfo& level) {
    RelationalSchema const* schema = relation_->GetSchema();
    for (auto& [column_combination, info] : level) {
        assert(info.rhs_candidates.any());
        // TODO: use nullptr as "this is a superkey". NOTE: in the case of 4.1 and 4.2 we may want
        // to delay actually setting PLI to nullptr to avoid direct checks on sibling keys.
        if (info.IsSuperkey() || !IsKey(info.pli.get())) {
            continue;
        }
        // The check in the original article is incorrect because we are not
        // guaranteed to have calculated C^+ of every sibling of the key.
        // But it is correct if we've kept the keys like here and in Metanome.
        info.MarkSuperkey();
        boost::dynamic_bitset<> sibling = column_combination;

        // By definition of C^+(X) an attribute A being in C^+(X) \ X means
        // ∀B ∈ X X \ {B} → {B} does not hold (A is taken out of the expression, because obviously
        // A ∈ X => A ∉ C^+(X) \ X), and vice versa.
        util::ForEachIndex(info.rhs_candidates, [&](model::Index rhs_index) {
            // rhs_candidates - column_combination without allocations
            if (column_combination.test(rhs_index)) return;
            sibling.set(rhs_index);
            for (model::Index i = column_combination.find_first();
                 i != boost::dynamic_bitset<>::npos; i = column_combination.find_next(i)) {
                sibling.reset(i);
                auto sibling_it = level.find(sibling);
                sibling.set(i);
                if (sibling_it == level.end() ||
                    !sibling_it->second.rhs_candidates.test(rhs_index)) {
                    sibling.reset(rhs_index);
                    return;
                }
            };
            RegisterAfd(AFD(schema->GetVertical(column_combination), *schema->GetColumn(rhs_index),
                            0.0 /* key -> attr is always a plain FD */,
                            relation_->GetSharedPtrSchema()));
            // Would not be a consideration if the nodes from the level were deleted, but since
            // we've found an FD, we need to delete it from the set of FDs that don't hold.
            // Otherwise, we may output incorrect results in the differently-sized keys case again.
            info.rhs_candidates.reset(rhs_index);
            sibling.reset(rhs_index);
        });
    }
}

void Tane::ComputeDependencies(LevelColumnCombinationsInfo& level,
                               LevelColumnCombinationsInfo const& prev_level) {
    RelationalSchema const* schema = relation_->GetSchema();

    for (auto it = level.begin(); it != level.end();) {
        auto& [column_combination, info] = *it;
        auto& [rhs_candidates, pli] = info;
        assert(rhs_candidates.any());
        if (info.IsSuperkey()) {
            ++it;
            continue;
        }
        if (column_combination.count() == 1) {
            model::Index const rhs_index = column_combination.find_first();
            if (!info.rhs_candidates.test(rhs_index)) {
                ++it;
                continue;
            }
            config::ErrorType error = CalculateZeroAryFdError(&relation_->GetColumnData(rhs_index));
            if (error > max_fd_error_) {
                ++it;
                continue;
            }
            RegisterAfd(
                    AFD(schema->GetVertical(boost::dynamic_bitset<>(relation_->GetNumColumns())),
                        *schema->GetColumn(rhs_index), error, relation_->GetSharedPtrSchema()));
            if (error == 0.0) {
                it = level.erase(it);
            } else {
                info.rhs_candidates.reset(rhs_index);
                if (info.rhs_candidates.none()) {
                    it = level.erase(it);
                } else {
                    ++it;
                }
            }
            continue;
        }
        boost::dynamic_bitset<> intersection = column_combination & info.rhs_candidates;
        boost::dynamic_bitset<> parent = column_combination;
        // TODO: the measures should be calculated at the same time as the PLIs are being
        // intersected instead of doing a separate pass, i.e. the intersected PLI should be created
        // here on the first iteration. Then it may be used for later iterations if there is a more
        // efficient way.
        // Creating the intersected PLIs here otherwise doesn't make sense, since the only effect it
        // will have is maybe allowing us to report more FDs before memory runs out, but the peak
        // memory usage would be the same either way.
        util::ForEachIndex(intersection, [&](model::Index rhs_index) {
            parent.reset(rhs_index);
            model::PLIWS const* lhs_pli = prev_level.find(parent)->second.pli.get();
            model::PLIWS const* rhs_pli = relation_->GetColumnData(rhs_index).GetPLWSIndex();
            config::ErrorType error = CalculateFdError(lhs_pli, rhs_pli, pli.get());
            if (error > max_fd_error_) {
                parent.set(rhs_index);
                return;
            }
            RegisterAfd(AFD(schema->GetVertical(parent), *schema->GetColumn(rhs_index), error,
                            relation_->GetSharedPtrSchema()));
            parent.set(rhs_index);
            info.rhs_candidates.reset(rhs_index);
            // This might be pointless, because these dependencies would be found for the siblings,
            // which would then have the previous line fire. These columns would not end up in the
            // intersections in the latter levels anyway. And because we are using bitsets to
            // calculate the intersection, instead of, say, iterating through C+^(X)'s columns, this
            // would not have any performance impact. Maybe we could get it to have some if we,
            // like, had bitsets smaller than 64 columns, then had checks that cut the number of
            // blocks if they are zero, but this is too difficult to implement and would probably
            // not have that much impact. In fact, the intersections might become slower. We've run
            // into a peculiarity with the machines we're using to compute: they only work with at
            // least 8 bits in parallel.
            // Except, not quite. The difference with the algorithm in the article is that we're
            // pruning empties immediately. If rhs_candidates becomes empty, we may delete it, which
            // could make some hashtable checks for a node's existence in GenerateNextLevel faster,
            // because a node at the hash(new_combination) index in the node array might not exist,
            // so we will avoid an equality check. However, I don't think it's going to happen that
            // much effect.
            if (error == 0.0) info.rhs_candidates &= column_combination;
        });
        if (info.rhs_candidates.none()) {
            it = level.erase(it);
        } else {
            ++it;
        }
    }
}

// Exactly PrefixBlocks but the order of bits is inverted and RHS candidates are stored.
auto Tane::SuffixBlocks(LevelColumnCombinationsInfo const& level) -> SuffixMap {
    SuffixMap map;
    for (auto const& [column_combination, info] : level) {
        assert(column_combination.size() >= 2);
        // Only one column combination with this suffix is possible, avoid adding.
        // TODO: test if this does anything.
        if (column_combination.test(0) && column_combination.test(1)) continue;
        boost::dynamic_bitset<> suffix = column_combination;
        model::Index const non_suffix_column = suffix.find_first();
        suffix.reset(non_suffix_column);
        map[std::move(suffix)].emplace_back(non_suffix_column, &info);
    }
    return map;
}

auto Tane::GenerateNextLevel(LevelColumnCombinationsInfo const& level)
        -> LevelColumnCombinationsInfo {
    LevelColumnCombinationsInfo next_level;
    SuffixMap s_map = SuffixBlocks(level);
    for (auto s_map_it = s_map.begin(); s_map_it != s_map.end();) {
        SuffixMap::node_type node = s_map.extract(s_map_it++);
        std::vector<NoSuffixColumnCombinationInfoRef>& prev_level_combinations = node.mapped();
        if (prev_level_combinations.size() < 2) continue;
        std::ranges::sort(prev_level_combinations, [](NoSuffixColumnCombinationInfoRef const& c1,
                                                      NoSuffixColumnCombinationInfoRef const& c2) {
            return c1.non_suffix_column < c2.non_suffix_column;
        });
        boost::dynamic_bitset<>& new_combination = node.key();
        for (auto outer_it = prev_level_combinations.begin(),
                  end_it = std::prev(prev_level_combinations.end());
             outer_it != end_it; ++outer_it) {
            new_combination.set(outer_it->non_suffix_column);
            for (auto inner_it = std::next(outer_it); inner_it != prev_level_combinations.end();
                 ++inner_it) {
                if (!outer_it->info->rhs_candidates.intersects(inner_it->info->rhs_candidates))
                    continue;
                boost::dynamic_bitset<> next_candidates =
                        inner_it->info->rhs_candidates & outer_it->info->rhs_candidates;
                bool is_superkey = outer_it->info->IsSuperkey() || inner_it->info->IsSuperkey();
                new_combination.set(inner_it->non_suffix_column);
                bool in_next_level = true;
                // No point in checking anything earlier, we already know they exist because we got
                // them from SuffixBlocks.
                for (model::Index i = new_combination.find_next(inner_it->non_suffix_column);
                     i != boost::dynamic_bitset<>::npos; i = new_combination.find_next(i)) {
                    new_combination.reset(i);
                    auto it = level.find(new_combination);
                    new_combination.set(i);
                    if (it == level.end() ||
                        (next_candidates &= it->second.rhs_candidates).none()) {
                        in_next_level = false;
                        break;
                    }
                    if (it->second.IsSuperkey()) is_superkey = true;
                }
                if (in_next_level) {
                    std::unique_ptr<model::PLIWS> pli =
                            is_superkey ? nullptr
                                        : outer_it->info->pli->Intersect(inner_it->info->pli.get());
                    next_level.try_emplace(new_combination, next_candidates, std::move(pli));
                }
                new_combination.reset(inner_it->non_suffix_column);
            }
            new_combination.reset(outer_it->non_suffix_column);
        }
    }
    return next_level;
}

config::ErrorType Tane::CalculateZeroAryFdError(ColumnData const* rhs) {
    // NOTE: Sometimes the empty LHS case is not defined, so we have to figure out a value that
    // makes sense on our own. If the RHS is constant, there is an FD, so it only makes sense for
    // error to be 0.
    switch (afd_measure_) {
        case model::AfdMeasure::kPerValue:
        case model::AfdMeasure::kG3: {
            std::size_t max = 1;
            model::PositionListIndex const* x_pli = rhs->GetPositionListIndex();
            for (Cluster const& x_cluster : x_pli->GetIndex()) {
                std::size_t const x_cluster_size = x_cluster.size();
                if (max < x_cluster_size) max = x_cluster_size;
            }
            return 1.0 - static_cast<config::ErrorType>(max) / x_pli->GetRelationSize();
        }
        case model::AfdMeasure::kG1:
            return afd_metric_calculator::AFDMetricCalculator::CalculateZeroAryG1(
                    rhs, relation_.get()->GetNumTuplePairs());
        /*
         * dom_{empty_set}(R) needs some care in its definition. If that care is taken, we get
         * |dom_{empty_set}(R)| = 1.
         */
        case model::AfdMeasure::kRho:
            return 1;  // incorrect
            /* return static_cast<config::ErrorType>(rhs->GetPositionListIndex()->GetNumCluster() -
             * 1) / rhs->GetPositionListIndex()->GetNumCluster(); */
        /*
         * The probability that a tuple participates in a violating pair is 0 if there is an FD,
         * otherwise it is 1 for an empty LHS and non-constant RHS
         */
        case model::AfdMeasure::kG2:
        /*
         * For an empty LHS, the mutual information is 0, but if the entropy of RHS is also 0 (i.e.
         * it is constant), the measure is technically undefined.
         */
        case model::AfdMeasure::kFi:
            return rhs->GetPositionListIndex()->IsConstant() ? 0.0 : 1.0;
        /*
         * The original definition of this one requires the presence of two attributes. The exact
         * expression used is pdep(X, Y) = p(R1.Y = R.Y2 | R1.X = R2.X). If we treat the projection
         * of a tuple on empty X as the empty set, then we get pdep({}, Y) = p(R1.Y = R.Y2), which
         * is exactly the self-dependency measure pdep(Y).
         */
        case model::AfdMeasure::kPdep:
            return 1;  // incorrect
            /*return 1 - afd_metric_calculator::AFDMetricCalculator::CalculatePdepSelf(
                               rhs->GetPLWSIndex());*/
        /*
         * When using pdep({}, Y) = pdep(Y), tau has 0 in the numerator. If pdep(Y), it has 0 in the
         * denominator too, so it is technically undefined.
         */
        case model::AfdMeasure::kTau:
        /*
         * Since Y is taken to be constant in the definition, pdep({} -> Y, R) is just pdep(Y). The
         * expected value of a constant is that constant, so we get a 0 in the numerator. The
         * denominator is again 0 when the RHS is constant, so this measure is technically undefined
         * as well in that case.
         */
        case model::AfdMeasure::kMuPlus:
            // return rhs->GetPositionListIndex()->IsConstant() ? 0.0 : 1.0;
            return 1;  // incorrect
    }
    assert(false);
    __builtin_unreachable();
}

config::ErrorType Tane::CalculateFdError(model::PLIWS const* lhs_pli, model::PLIWS const* rhs_pli,
                                         model::PLIWS const* joint_pli) {
    switch (afd_measure_) {
        case model::AfdMeasure::kPdep:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculatePdepMeasure(lhs_pli,
                                                                                        joint_pli);
        case model::AfdMeasure::kTau:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateTau(lhs_pli, rhs_pli,
                                                                                joint_pli);
        case model::AfdMeasure::kMuPlus:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateMuPlus(lhs_pli, rhs_pli,
                                                                                   joint_pli);
        case model::AfdMeasure::kRho:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateRhoMeasure(lhs_pli,
                                                                                       joint_pli);
        case model::AfdMeasure::kFi:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateFI(
                               lhs_pli, rhs_pli, relation_.get()->GetNumTuplePairs());
        case model::AfdMeasure::kG2:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateG2(
                               lhs_pli, rhs_pli, relation_.get()->GetNumTuplePairs());
        case model::AfdMeasure::kG3:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateG3(
                               lhs_pli, rhs_pli, relation_.get()->GetNumTuplePairs());
        case model::AfdMeasure::kG1:
            return afd_metric_calculator::AFDMetricCalculator::CalculateG1Error(
                    lhs_pli, joint_pli, relation_.get()->GetNumTuplePairs());
        case model::AfdMeasure::kPerValue:
            return 1.0 - afd_metric_calculator::AFDMetricCalculator::CalculatePerValue(lhs_pli,
                                                                                       joint_pli);
    }
    assert(false);
    __builtin_unreachable();
}

void Tane::ExecuteInternal() {
    if (relation_->GetNumColumns() < 2) return;
    LevelColumnCombinationsInfo prev_level;
    prev_level.try_emplace(boost::dynamic_bitset<>(relation_->GetNumColumns()),
                           std::move(boost::dynamic_bitset<>(relation_->GetNumColumns()).set()));
    LevelColumnCombinationsInfo current_level;
    for (model::Index column_index = 0; column_index != relation_->GetNumColumns();
         ++column_index) {
        boost::dynamic_bitset<> column_combination(relation_->GetNumColumns());
        column_combination.set(column_index);
        current_level.try_emplace(
                std::move(column_combination),
                std::move(boost::dynamic_bitset<>(relation_->GetNumColumns()).set()),
                std::make_unique<model::PLIWS>(
                        *relation_->GetColumnData(column_index).GetPLWSIndex()));
    }
    std::size_t level_index = 0;
    while (!current_level.empty()) {
        ComputeDependencies(current_level, prev_level);
        if (level_index == max_lhs_) break;
        Prune(current_level);

        prev_level = std::move(current_level);
        // TODO: On max_lhs_ level, do not save PLIs. This will need code that differs more from
        // what's here right now, but once we have better measures calculation (i.e. intersections
        // happen in ComputeDependencies), the procedure is not going to be as different.
        current_level = GenerateNextLevel(prev_level);
        ++level_index;
    }
}

}  // namespace algos
