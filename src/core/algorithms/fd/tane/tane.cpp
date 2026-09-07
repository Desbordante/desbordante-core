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

namespace {
bool IsKey(model::PositionListIndex const* pli) {
    return pli->AllValuesAreUnique();
}
}  // namespace

namespace algos {
// This implements TANE from "TANE: An Efficient Algorithm for Discovering Functional and
// Approximate Dependencies" by Ykä Huhtala, Juha Kärkkäinen, Pasi Porkka, and Hannu Toivonen.
// The algorithm as described in the article specifies or mentions 7 procedures: TANE itself,
// GENERATE_NEXT_LEVEL, PREFIX_BLOCKS, COMPUTE_DEPENDENCIES, PRUNE, STRIPPED_PRODUCT, e. Whenever
// these names are used in the comments, the corresponding pseudocode from the article is
// referenced. A comment in the form of /* PRUNE, 6 */ means the code corresponds to line 6 of the
// PRUNE procedure.
//
// This implementation has some modifications for the purposes of code reuse, correctness, and
// optimization.
//
// Firstly, the procedures STRIPPED_PRODUCT and e are not implemented here, and the corresponding
// parts use this project's existing code.
//
// Secondly, the "Bounding e" modification (3.3.2) is not used. This lets us use this algorithm to
// discover AFDs with arbitrary AFD measures without much extra effort: otherwise we would need to
// figure out how to sensibly define e for standalone attribute sets for each measure, and it is not
// guaranteed that the bounds would hold up. Right now, the only requirement on the function
// measuring the error is that it must return 0.0 if the FD is exact.
//
// Most importantly, the algorithm as described in the paper is incorrect, because it deletes
// superkey nodes too eagerly, leading to failures when the intersection on line 6 of PRUNE is
// checked, as we might not have one or more of the intersected C^+ sets calculated. This shows
// itself when there is a column in C^+(X) \ X of a key (iterated in line 5 of PRUNE) that belongs
// to another key of smaller size, as said key's ancestor would have been deleted when processing a
// previous level, with line 2 in COMPUTE_DEPENDENCIES consequently not calculating the further C^+
// sets.
//
// This implementation does not delete key attribute sets (line 8 of PRUNE) from the level but
// instead marks them as superkeys. The marked attribute sets then get skipped, conceptually, after
// a check for the mark that would be after line 3 of COMPUTE_DEPENDENCIES, while lines 1 and 2 are
// executed normally. The mark is spread in GENERATE_NEXT_LEVEL, where if one direct subset (line 5)
// has it, the attribute set on the new level has it as well. Metanome's implementation of TANE does
// this too.
//
// When implemented this way, it makes sense to store all C^+(X) together with X in a single
// dictionary, along with the corresponding PLI. The mark doesn't have to be stored, because PLIs
// are only needed for error calculations, and they are never performed when the attribute set is a
// superkey. Therefore, we can use the absence of a PLI as the superkey mark.
//
// The absence of C^+(X) can also be dealt with by directly checking each X \ {B} -> A (B ∈ X, A ∉
// X) dependency when processing a key in PRUNE if no other C^+(X ∪ {A} \ {B}) (line 6) that doesn't
// contain A exists (i. e. no evidence if X \ {B} -> A holds). If we do happen upon it, we know that
// X \ {B} -> A holds by definition of C^+(X) and don't have to check that. There would be a PLI
// available for each X \ {B}, as those had to have been part of the previous level for X to end up
// in the current one. Line 8 would still be there. If this is used with the current single map, the
// deletion of an attribute set would mean deleting its PLI and C^+(X). There could be some
// variation on when exactly this is done, either by postponing the deletion until the end of PRUNE
// or by separating PLIs and candidates from the level's combinations.
// TODO: try implementing the above as well.
//
// Because C^+(X) sets are stored together with X, they are calculated in GENERATE_NEXT_LEVEL, which
// is equivalent to what is described in the article, as COMPUTE_DEPENDENCIES follows
// GENERATE_NEXT_LEVEL if the loop condition holds (TANE lines 6, 8, 5 respectively). If it doesn't,
// then COMPUTE_DEPENDENCIES doesn't get executed due to the level being empty, and the same happens
// here. In the situation where we check every bit in line 5 of GENERATE_NEXT_LEVEL, but the
// attribute set with the last one excluded is not present, we have done useless intersection work,
// but I don't think it's that much.
//
// In addition to that, the check in lines 2 and 3 of PRUNE is done throughout the algorithm: if
// C^+(X) becomes empty in GENERATE_NEXT_LEVEL (with the intersection above), it would have no
// effect in COMPUTE_DEPENDENCIES and would be deleted in a later PRUNE. If it becomes empty in
// COMPUTE_DEPENDENCIES it would have no effect and be deleted in PRUNE once again.

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

void Tane::Prune(LevelAttributeSetsData& level) {
    RelationalSchema const* schema = relation_->GetSchema();
    /* PRUNE, 1 */
    for (auto& [column_combination, info] : level) {
        assert(info.rhs_candidates.any());
        if (info.IsSuperkey() || /* PRUNE, 4 */ !IsKey(info.pli.get())) {
            continue;
        }
        // Only executed if we have a key, superkeys have already been processed.

        boost::dynamic_bitset<> sibling = column_combination;

        // By definition of C^+(X) an attribute A being in C^+(X) \ X means
        // ∀B ∈ X X \ {B} → {B} does not hold (A is taken out of the expression, because obviously
        // A ∈ X => A ∉ C^+(X) \ X). If there was no early key check, this condition would be
        // checked when processing the next level once C^+ sets would have been intersected (lines 4
        // and 5 of COMPUTE_DEPENDENCIES for a few attribute sets). So here we are essentially
        // calculating them early for the keys. If this had worked correctly and we could remove
        // keys' attribute sets, then we would be able to avoid calculations for attribute sets that
        // are parents. However, we still are avoiding intersecting the PLIs, which probably matters
        // a lot more in practice than a few extra ANDs on the bitsets, even if those theoretically
        // grow exponentially in number.
        /* PRUNE, 5 */
        util::ForEachIndex(info.rhs_candidates, [&](model::Index rhs_index) {
            // info.rhs_candidates - column_combination without allocations
            if (column_combination.test(rhs_index)) return;
            sibling.set(rhs_index);
            /* PRUNE, 6 */
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
            sibling.reset(rhs_index);
            /* PRUNE, 7 */
            RegisterAfd(AFD(schema->GetVertical(column_combination), *schema->GetColumn(rhs_index),
                            0.0 /* key -> attr is always a plain FD */,
                            relation_->GetSharedPtrSchema()));

            // Would not be a consideration if the nodes from the level were deleted, but since
            // we've found an FD, we need to delete it from the set of FDs that don't hold.
            // Otherwise, we may output incorrect results in the differently-sized keys case.
            info.rhs_candidates.reset(rhs_index);
            // Sanity check: this reset will not affect the procedure for the following sibling
            // keys, because A is inside (X ∪ {A} \ {B}), and if that is a sibling key, the
            // procedure will iterate through C+(X ∪ {A} \ {B}) \ (X ∪ {A} \ {B}) when it is
            // reached, which will not check A in this key's C^+(X).
        });

        // Roughly equivalent to line 8 of PRUNE.
        info.MarkSuperkey();
    }
}

void Tane::ComputeDependencies(LevelAttributeSetsData& level,
                               LevelAttributeSetsData const& prev_level) {
    RelationalSchema const* schema = relation_->GetSchema();

    /* COMPUTE_DEPENDENCIES, 3 */
    for (auto it = level.begin(); it != level.end();) {
        auto& [column_combination, info] = *it;
        auto& [rhs_candidates, pli] = info;
        assert(rhs_candidates.any());
        if (info.IsSuperkey()) {
            ++it;
            continue;
        }
        assert(column_combination.count() > 1);
        boost::dynamic_bitset<> lhs_attribute_mask = column_combination;
        // TODO: the measures should be calculated at the same time as the PLIs are being
        // intersected instead of doing a separate pass, i.e. the intersected PLI should be created
        // here on the first iteration. Then it may be used for later iterations if there is a more
        // efficient way.
        // Creating the intersected PLIs here otherwise doesn't make sense, since the only effect it
        // will have is maybe allowing us to report more FDs before memory runs out, but the peak
        // memory usage would be the same either way.
        /* COMPUTE_DEPENDENCIES, 4 */
        util::ForEachIndex(info.rhs_candidates, [&](model::Index rhs_index) {
            // info.rhs_candidates & column_combination without allocations
            if (!column_combination.test(rhs_index)) return;
            /* COMPUTE_DEPENDENCIES, 5' */
            lhs_attribute_mask.reset(rhs_index);
            model::PLIWS const* lhs_pli = prev_level.find(lhs_attribute_mask)->second.pli.get();
            model::PLIWS const* rhs_pli = relation_->GetColumnData(rhs_index).GetPLWSIndex();
            config::ErrorType error = CalculateFdError(lhs_pli, rhs_pli, pli.get());
            if (error > max_fd_error_) {
                lhs_attribute_mask.set(rhs_index);
                return;
            }
            /* COMPUTE_DEPENDENCIES, 6 */
            RegisterAfd(AFD(schema->GetVertical(lhs_attribute_mask), *schema->GetColumn(rhs_index),
                            error, relation_->GetSharedPtrSchema()));

            lhs_attribute_mask.set(rhs_index);
            /* COMPUTE_DEPENDENCIES, 7 */
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
            /* COMPUTE_DEPENDENCIES, 8'-9' */
            if (error == 0.0) info.rhs_candidates &= column_combination;
        });
        /* PRUNE, 2 */
        if (info.rhs_candidates.none()) {
            /* PRUNE, 3 */
            it = level.erase(it);
        } else {
            ++it;
        }
    }
}

// Exactly PREFIX_BLOCKS but the order of bits is inverted and RHS candidates are stored.
auto Tane::SuffixBlocks(LevelAttributeSetsData const& level) -> SuffixMap {
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

auto Tane::GenerateNextLevel(LevelAttributeSetsData const& level) -> LevelAttributeSetsData {
    /* GENERATE_NEXT_LEVEL, 1 */
    LevelAttributeSetsData next_level;
    SuffixMap s_map = SuffixBlocks(level);
    /* GENERATE_NEXT_LEVEL, 2 */
    for (auto s_map_it = s_map.begin(); s_map_it != s_map.end();) {
        SuffixMap::node_type node = s_map.extract(s_map_it++);
        std::vector<NoSuffixAttributeSetDataReference>& prev_level_combinations = node.mapped();
        if (prev_level_combinations.size() < 2) continue;
        boost::dynamic_bitset<>& new_combination = node.key();
        /* GENERATE_NEXT_LEVEL, 3 */
        for (auto outer_it = prev_level_combinations.begin(),
                  end_it = std::prev(prev_level_combinations.end());
             outer_it != end_it; ++outer_it) {
            /* GENERATE_NEXT_LEVEL, 4 */
            new_combination.set(outer_it->non_suffix_column);
            for (auto inner_it = std::next(outer_it); inner_it != prev_level_combinations.end();
                 ++inner_it) {
                /* PRUNE, 2-3 */
                if (!outer_it->info->rhs_candidates.intersects(inner_it->info->rhs_candidates))
                    continue;
                /* GENERATE_NEXT_LEVEL, 4 */
                new_combination.set(inner_it->non_suffix_column);
                /* COMPUTE_DEPENDENCIES, 2 */
                boost::dynamic_bitset<> next_candidates =
                        inner_it->info->rhs_candidates & outer_it->info->rhs_candidates;
                bool is_superkey = inner_it->info->IsSuperkey() || outer_it->info->IsSuperkey();

                bool in_next_level = true;
                // No point in checking anything earlier, we already know they exist because we got
                // them from SuffixBlocks.
                /* GENERATE_NEXT_LEVEL, 5 */
                for (model::Index i = new_combination.find_next(inner_it->non_suffix_column);
                     i != boost::dynamic_bitset<>::npos; i = new_combination.find_next(i)) {
                    new_combination.reset(i);
                    auto it = level.find(new_combination);
                    new_combination.set(i);
                    if (it == level.end() ||
                        (/* COMPUTE_DEPENDENCIES, 2 */ next_candidates &= it->second.rhs_candidates)
                                /* PRUNE, 2-3 */.none()) {
                        in_next_level = false;
                        break;
                    }
                    if (it->second.IsSuperkey()) is_superkey = true;
                }
                if (in_next_level) {
                    std::unique_ptr<model::PLIWS> pli =
                            is_superkey ? nullptr
                                        : outer_it->info->pli->Intersect(inner_it->info->pli.get());
                    /* GENERATE_NEXT_LEVEL, 6 */
                    next_level.try_emplace(new_combination, next_candidates, std::move(pli));
                }
                new_combination.reset(inner_it->non_suffix_column);
            }
            new_combination.reset(outer_it->non_suffix_column);
        }
    }
    return next_level;
}

// Roughly the e procedure for LHS = ∅
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
            return static_cast<config::ErrorType>(rhs->GetPositionListIndex()->GetNumCluster() -
                                                  1) /
                   rhs->GetPositionListIndex()->GetNumCluster();
        /*
         * The original definition of this one requires the presence of two attributes. The exact
         * expression used is pdep(X, Y) = p(R1.Y = R.Y2 | R1.X = R2.X). If we treat the projection
         * of a tuple on empty X as the empty set, then we get pdep({}, Y) = p(R1.Y = R.Y2), which
         * is exactly the self-dependency measure pdep(Y).
         */
        case model::AfdMeasure::kPdep:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculatePdepSelf(
                               rhs->GetPLWSIndex());
        /*
         * The probability that a tuple participates in a violating pair is 0 if there is an FD,
         * otherwise it is 1 for an empty LHS and non-constant RHS.
         */
        case model::AfdMeasure::kG2:
        /*
         * For an empty LHS, the mutual information is 0, but if the entropy of RHS is also 0 (i.e.
         * it is constant), the measure is technically undefined.
         */
        case model::AfdMeasure::kFi:
        /*
         * When using pdep({}, Y) = pdep(Y), tau has 0 in the numerator. If pdep(Y) = 0, it has 0 in
         * the denominator too, so it is technically undefined.
         */
        case model::AfdMeasure::kTau:
        /*
         * Since Y is taken to be constant in the definition, pdep({} -> Y, R) is just pdep(Y). The
         * expected value of a constant is that constant, so we get a 0 in the numerator. The
         * denominator is again 0 when the RHS is constant, so this measure is technically undefined
         * as well in that case.
         */
        case model::AfdMeasure::kMuPlus:
            return rhs->GetPositionListIndex()->IsConstant() ? 0.0 : 1.0;
    }
    assert(false);
    __builtin_unreachable();
}

// Roughly the e procedure for LHS != ∅
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

// COMPUTE_DEPENDENCIES(L_1)
// Lines 1 and 2 are pointless, C^+({A}) is exactly R at this point
// Lines 3 and 4 reduce to "for each attribute"
// Line 5 is an error calculation for an FD with an empty LHS.
// If the condition on line 8' holds, the attribute set will be pruned (C^+({A}) = ∅), which is
// exactly the same as if there was no such attribute set in the first place, so we can just not add
// it.
auto Tane::ComputeDependenciesLevel1() -> FirstLevelComputeDependenciesResult {
    RelationalSchema const* schema = relation_->GetSchema();
    std::vector<model::Index> inexact_zeroary_afd_rhss;  // C^+({A}) = R \ {A}
    std::vector<model::Index> not_zeroary_afd_rhss;      // C^+({A}) = R
    // These are removed from all C^+(X) immediately. Without that, line 6 of PRUNE has to go
    // through them every time in line 6's intersection.
    // Idea: if we take FDs with a column as their RHS and look at their LHSs, if no LHS on the
    // further levels can form a minimal FD, we can delete it from all C^+(X). Can we perhaps detect
    // this situation during execution at a low cost?
    boost::dynamic_bitset not_exact_zeroary_afd_rhss = CreateEmptyColumnMask();
    not_exact_zeroary_afd_rhss.set();
    /* COMPUTE_DEPENDENCIES, 3-4 */
    for (model::Index col = 0; col != relation_->GetNumColumns(); ++col) {
        // X = {col}
        /* COMPUTE_DEPENDENCIES, 5 */
        ColumnData const& column_data = relation_->GetColumnData(col);
        double fd_error = CalculateZeroAryFdError(&column_data);
        // if X \ {A} → A is valid
        if (fd_error <= max_fd_error_) {
            /* COMPUTE_DEPENDENCIES, 6 */
            RegisterAfd(AFD(schema->CreateEmptyVertical(), *schema->GetColumn(col), fd_error,
                            relation_->GetSharedPtrSchema()));
            /* COMPUTE_DEPENDENCIES, 8' */
            if (fd_error == 0) {
                /* COMPUTE_DEPENDENCIES, 7, 9' */
                not_exact_zeroary_afd_rhss.reset(col);  // C^+({A}) = ∅
                /* PRUNE, 2-3 */
            } else {
                /* COMPUTE_DEPENDENCIES, 7 */
                inexact_zeroary_afd_rhss.push_back(col);  // C^+({A}) = R \ {A}
            }
            continue;
        }
        not_zeroary_afd_rhss.push_back(col);  // C^+({A}) = R
    }

    return {std::move(inexact_zeroary_afd_rhss), std::move(not_zeroary_afd_rhss),
            std::move(not_exact_zeroary_afd_rhss)};
}

// Note that in a table of size 1, all FDs hold exactly, once this is called we can assume that
// some AFDs don't hold. And that the size of the table is greater than 1.
// We can assume that a column cannot be both a key and zeroary FD RHS:
// If it's a zeroary FD RHS, then all its values are equal unconditionally.
// If it's a key, then either the table does not have records that differ, or it has more than
// one distinct value. If the table does not have records that differ, then all the zeroary FDs
// hold, which we know is not true by this point. Therefore, the column cannot be a zeroary FD
// RHS.
// The only possible values for C^+({A}) for {A} that is in the level at this point are R \ {A}
// and R (minus zeroary RHSs).
// PRUNE(L_1)
// Lines 2 and 3 can be omitted as attributes with C^+({A}) = ∅ have not been added.
// Line 4: keep as is
// C^+({A}) \ {A} is:
// (not_zeroary_afd_rhss) 1. C^+({A}) = R => C^+({A}) \ {A} = R \ {A}
// (inexact_zeroary_afd_rhss) 2. C^+({A}) = R \ {A} => C^+({A}) \ {A} = R \ {A}
// Thus, line 5 is iteration over all attributes except the one in X.
// The intersection on line 6 is computed for one element, so it is that element.
// C^+({B} ∪ {A} \ {B}) = C^+({A}), the cases are listed above.
// The condition on line 6 on L_1 holds in case 1 and doesn't otherwise. Case 1 is the one where
// the attribute is not an RHS of an FD (exact or approximate).
// So, for case 1, lines 5 and 6 mean iteration over all attributes that are not valid RHSs and
// not the element in X. The key is marked and its C^+(X) is R \ ({A | C^+({A}) = R} \ X).
// For case 2, lines 5 and 6 also mean iteration over those attributes (and the one in X is not
// one of them). The key is marked and its C^+(X) is (R \ {A | C^+({A}) = R}.
// Therefore, the possible values of C^+({A}) after PRUNE(L_1) are:
// 1. Not AFD RHSs, keys = R: C^+({A}) = R \ {B | C^+({B}) = R /\ B != A}
// 2. AFD RHSs with non-zero error, keys: C^+({A}) = R \ {A | C^+({A}) = R}
// 3. AFD RHSs with non-zero error, not keys: C^+({A}) = R \ {A}
// 4. Not AFD RHSs, not keys: C^+({A}) = R
// Any FD from key to a column has error 0.0, so that's output.
auto Tane::PruneLevel1(std::vector<model::Index> const& inexact_zeroary_afd_rhss,
                       std::vector<model::Index> const& not_zeroary_afd_rhss,
                       boost::dynamic_bitset<> const& not_exact_zeroary_afd_rhss)
        -> FirstLevelPruneResults {
    RelationalSchema const* schema = relation_->GetSchema();

    std::vector<model::Index> non_key_attrs_not_0afd;  // C^+({A}) = R
    std::vector<model::Index> key_attrs_not_0afd;      // C^+({B}) = R \ ({A | C^+({A}) = R} \ {B})
    boost::dynamic_bitset<> superkey_rhs_candidates = not_exact_zeroary_afd_rhss;
    /* PRUNE, 1 */
    for (auto lhs_it = not_zeroary_afd_rhss.begin(); lhs_it != not_zeroary_afd_rhss.end();) {
        superkey_rhs_candidates.reset(*lhs_it);
        model::Index const not_zeroary_afd_rhs = *lhs_it;
        /* PRUNE, 4 */
        if (!IsKey(relation_->GetColumnData(not_zeroary_afd_rhs).GetPositionListIndex())) {
            non_key_attrs_not_0afd.push_back(not_zeroary_afd_rhs);
            ++lhs_it;
            continue;
        }
        /* PRUNE, 5-6 */
        for (auto rhs_it = not_zeroary_afd_rhss.begin(); rhs_it != lhs_it; ++rhs_it) {
            /* PRUNE, 7 */
            RegisterAfd(AFD(schema->GetVertical(
                                    std::move(CreateEmptyColumnMask().set(not_zeroary_afd_rhs))),
                            *schema->GetColumn(*rhs_it), 0.0, relation_->GetSharedPtrSchema()));
        }
        /* PRUNE, 5-6 */
        for (auto rhs_it = ++lhs_it; rhs_it != not_zeroary_afd_rhss.end(); ++rhs_it) {
            /* PRUNE, 7 */
            RegisterAfd(AFD(schema->GetVertical(
                                    std::move(CreateEmptyColumnMask().set(not_zeroary_afd_rhs))),
                            *schema->GetColumn(*rhs_it), 0.0, relation_->GetSharedPtrSchema()));
        }
        // Roughly line 8 of PRUNE
        key_attrs_not_0afd.push_back(not_zeroary_afd_rhs);
    }

    std::vector<model::Index> non_key_attrs_0afd;  // C^+({A}) = R \ {A}
    std::vector<model::Index> key_attrs_0afd;      // C^+({A}) = R \ {A | C^+({A}) = R}
    for (model::Index inexact_zeroary_afd_rhs : inexact_zeroary_afd_rhss) {
        /* PRUNE, 4 */
        if (!IsKey(relation_->GetColumnData(inexact_zeroary_afd_rhs).GetPositionListIndex())) {
            non_key_attrs_0afd.push_back(inexact_zeroary_afd_rhs);
            continue;
        }
        /* PRUNE, 5-6 */
        for (model::Index not_zeroary_afd_rhs : not_zeroary_afd_rhss) {
            /* PRUNE, 7 */
            RegisterAfd(AFD(
                    schema->GetVertical(
                            std::move(CreateEmptyColumnMask().set(inexact_zeroary_afd_rhs))),
                    *schema->GetColumn(not_zeroary_afd_rhs), 0.0, relation_->GetSharedPtrSchema()));
        }
        // Roughly line 8 of PRUNE
        key_attrs_0afd.push_back(inexact_zeroary_afd_rhs);
    }

    return {std::move(non_key_attrs_not_0afd), std::move(key_attrs_not_0afd),
            std::move(non_key_attrs_0afd), std::move(key_attrs_0afd),
            std::move(superkey_rhs_candidates)};
}

// Case 1: key_attrs_not_0afd, non_key_attrs_not_0afd
// C^+(X) = (R \ ({A | C^+({A}) = R} \ {B})) ⋂ R = R \ ({A | C^+({A}) = R} \ {B})
//
// Case 2: key_attrs_not_0afd, non_key_attrs_0afd
// C^+(X) = R \ ({A | C^+({A}) = R} \ {B}) ⋂ (R \ {C}) = R \ ({A | C^+({A}) = R} \ {B}) \ {C}
//
// Case 3: key_attrs_not_0afd, key_attrs_0afd
// C^+(X) = R \ ({A | C^+({A}) = R} \ {B}) ⋂ (R \ {A | C^+({A}) = R}) = R \ {A | C^+({A}) = R}
//
// Case 4: key_attrs_not_0afd, key_attrs_not_0afd
// C^+(X) = (R \ ({A | C^+({A}) = R} \ {B})) ⋂ (R \ ({A | C^+({A}) = R} \ {C})) =
//   R \ {A | C^+({A}) = R}
void Tane::ComputeDependenciesLevel2KeysNotZeroaryAfdRhs(
        std::vector<model::Index> const& non_key_attrs_not_0afd,
        std::vector<model::Index> const& key_attrs_not_0afd,
        std::vector<model::Index> const& non_key_attrs_0afd,
        std::vector<model::Index> const& key_attrs_0afd,
        boost::dynamic_bitset<>& superkey_rhs_candidates, LevelAttributeSetsData& current_level) {
    for (auto it = key_attrs_not_0afd.begin(); it != key_attrs_not_0afd.end();) {
        model::Index const key_attr = *it++;
        auto add_superkey = [&](model::Index other_attr) {
            current_level.try_emplace(
                    std::move(CreateEmptyColumnMask().set(key_attr).set(other_attr)),
                    superkey_rhs_candidates);
        };
        assert(!superkey_rhs_candidates.test(key_attr));
        superkey_rhs_candidates.set(key_attr);
        // Case 1
        for (model::Index i : non_key_attrs_not_0afd) {
            add_superkey(i);
        }
        // Case 2
        for (model::Index i : non_key_attrs_0afd) {
            assert(superkey_rhs_candidates.test(i));
            superkey_rhs_candidates.reset(i);
            add_superkey(i);
            superkey_rhs_candidates.set(i);
        }
        superkey_rhs_candidates.reset(key_attr);
        if (superkey_rhs_candidates.any()) {
            // Case 3
            for (model::Index i : key_attrs_0afd) {
                add_superkey(i);
            }
            // Case 4
            for (auto it2 = it; it2 != key_attrs_not_0afd.end(); ++it2) {
                add_superkey(*it2);
            }
        }
    }
}

// Case 5: key_attrs_0afd, non_key_attrs_0afd
// C^+(X) = (R \ {A | C^+({A}) = R}) ⋂ (R \ {C}) = (R \ {A | C^+({A}) = R}) \ {C}
//
// Case 6: key_attrs_0afd, key_attrs_0afd
// C^+(X) = (R \ {A | C^+({A}) = R}) ⋂ (R \ {A | C^+({A}) = R}) = R \ {A | C^+({A}) = R}
//
// Case 7: key_attrs_0afd, non_key_attrs_not_0afd
// C^+(X) = (R \ {A | C^+({A}) = R}) ⋂ R = R \ {A | C^+({A}) = R}
void Tane::ComputeDependenciesLevel2KeysZeroaryAfdRhs(
        std::vector<model::Index> const& non_key_attrs_not_0afd,
        std::vector<model::Index> const& non_key_attrs_0afd,
        std::vector<model::Index> const& key_attrs_0afd,
        boost::dynamic_bitset<>& superkey_rhs_candidates, LevelAttributeSetsData& current_level) {
    for (auto it = key_attrs_0afd.begin(); it != key_attrs_0afd.end();) {
        model::Index const key_attr = *it++;
        auto add_superkey = [&](model::Index other_attr) {
            current_level.try_emplace(
                    std::move(CreateEmptyColumnMask().set(key_attr).set(other_attr)),
                    superkey_rhs_candidates);
        };
        // Case 5
        for (model::Index i : non_key_attrs_0afd) {
            assert(superkey_rhs_candidates.test(i));
            superkey_rhs_candidates.reset(i);
            add_superkey(i);
            superkey_rhs_candidates.set(i);
        }
        if (superkey_rhs_candidates.any()) {
            // Case 6
            for (auto it2 = it; it2 != key_attrs_0afd.end(); ++it2) {
                add_superkey(*it2);
            }
            // Case 7
            for (model::Index i : non_key_attrs_not_0afd) {
                add_superkey(i);
            }
        }
    }
}

// ComputeDependencies skips superkeys, so no FDs have to be accounted for, so the final C^+(X)
// is just the intersection of the corresponding columns' C^+(X) (see above).
void Tane::ComputeDependenciesLevel2Keys(std::vector<model::Index> const& non_key_attrs_not_0afd,
                                         std::vector<model::Index> const& key_attrs_not_0afd,
                                         std::vector<model::Index> const& non_key_attrs_0afd,
                                         std::vector<model::Index> const& key_attrs_0afd,
                                         boost::dynamic_bitset<>& superkey_rhs_candidates,
                                         LevelAttributeSetsData& current_level) {
    ComputeDependenciesLevel2KeysNotZeroaryAfdRhs(non_key_attrs_not_0afd, key_attrs_not_0afd,
                                                  non_key_attrs_0afd, key_attrs_0afd,
                                                  superkey_rhs_candidates, current_level);
    ComputeDependenciesLevel2KeysZeroaryAfdRhs(non_key_attrs_not_0afd, non_key_attrs_0afd,
                                               key_attrs_0afd, superkey_rhs_candidates,
                                               current_level);
}

// Case 8: non_key_attrs_0afd, non_key_attrs_0afd
// C^+(X) = (R \ {A}) ⋂ (R \ {B}) = R \ {A, B}
void Tane::ComputeDependenciesLevel2NonKeysZeroaryAfdRhsPairs(
        boost::dynamic_bitset<> const& not_exact_zeroary_afd_rhss,
        std::vector<model::Index> const& non_key_attrs_0afd,
        LevelAttributeSetsData& current_level) {
    // The intersection in Line 4 of COMPUTE_DEPENDENCIES is empty, C^+(X) is from just
    // GENERATE_NEXT_LEVEL.
    for (auto attr_it = non_key_attrs_0afd.begin(); attr_it != non_key_attrs_0afd.end();
         ++attr_it) {
        model::Index const col1 = *attr_it;
        for (auto attr2_it = std::next(attr_it); attr2_it != non_key_attrs_0afd.end(); ++attr2_it) {
            model::Index const col2 = *attr2_it;
            current_level.try_emplace(std::move(CreateEmptyColumnMask().set(col1).set(col2)),
                                      std::move(boost::dynamic_bitset(not_exact_zeroary_afd_rhss)
                                                        .reset(col1)
                                                        .reset(col2)),
                                      relation_->GetColumnData(col1).GetPLWSIndex()->Intersect(
                                              relation_->GetColumnData(col2).GetPLWSIndex()));
        }
    }
}

// Unlike the other cases, we can find new FDs here, C^+(X)'s can get modified further in what
// corresponds to steps from COMPUTE_DEPENDENCIES.
//
// Case 9: non_key_attrs_0afd, non_key_attrs_not_0afd
// C^+(X) = (R \ {A}) ⋂ R = R \ {A}
//
// Case 10: non_key_attrs_not_0afd, non_key_attrs_not_0afd
// C^+(X) = R ⋂ R = R
void Tane::ComputeDependenciesLevel2NonKeysNotBothZeroaryAfdRhs(
        boost::dynamic_bitset<> const& not_exact_zeroary_afd_rhss,
        std::vector<model::Index> const& non_key_attrs_not_0afd,
        std::vector<model::Index> const& non_key_attrs_0afd,
        LevelAttributeSetsData& current_level) {
    RelationalSchema const* schema = relation_->GetSchema();

    for (auto col1_it = non_key_attrs_not_0afd.begin(); col1_it != non_key_attrs_not_0afd.end();) {
        model::Index const col1 = *col1_it;
        // Case 9
        for (model::Index const col2 : non_key_attrs_0afd) {
            // C^+(X) = (R \ {col2})
            // X ∩ C^+(X) = {col1, col2} ∩ (R \ {col2}) = {col1}
            // Check: col2 -> col1
            model::PLIWS const* pli1 = relation_->GetColumnData(col1).GetPLWSIndex();
            model::PLIWS const* pli2 = relation_->GetColumnData(col2).GetPLWSIndex();
            std::unique_ptr<model::PLIWithSingletons> joint_pli = pli1->Intersect(pli2);

            config::ErrorType error21 = CalculateFdError(pli2, pli1, joint_pli.get());
            auto reg21 = [&]() {
                RegisterAfd(AFD(schema->GetVertical(std::move(CreateEmptyColumnMask().set(col2))),
                                *schema->GetColumn(col1), error21,
                                relation_->GetSharedPtrSchema()));
            };
            if (error21 == 0.0) {
                // output X \ {A} → A
                reg21();
                // 7 remove A from C+(X)
                // 9' remove all B in R \ X from C+(X) <=> intersect C^+(X) with X
                // C^+({col1, col2}) = ((R \ {col2}) \ {col1}) ∩ {col1, col2} = ∅
                continue;
            }
            boost::dynamic_bitset<> rhs_candidates = not_exact_zeroary_afd_rhss;
            rhs_candidates.reset(col2);
            assert(rhs_candidates.test(col1));
            // if e(X \ {A} → A) ≤ ε then
            if (error21 <= max_fd_error_) {
                // output X \ {A} → A
                reg21();
                // remove A from C+(X)
                rhs_candidates.reset(col1);
                // TODO: is this possible?
                if (rhs_candidates.none()) continue;
            }
            current_level.try_emplace(std::move(CreateEmptyColumnMask().set(col1).set(col2)),
                                      std::move(rhs_candidates), std::move(joint_pli));
        }
        // Case 10
        for (auto col2_it = ++col1_it; col2_it != non_key_attrs_not_0afd.end(); ++col2_it) {
            model::Index const col2 = *col2_it;

            // C^+(X) = R
            // Check col1 -> col2, col2 -> col1
            model::PLIWS const* pli1 = relation_->GetColumnData(col1).GetPLWSIndex();
            model::PLIWS const* pli2 = relation_->GetColumnData(col2).GetPLWSIndex();
            std::unique_ptr<model::PLIWithSingletons> joint_pli = pli1->Intersect(pli2);

            config::ErrorType error12 = CalculateFdError(pli1, pli2, joint_pli.get());
            config::ErrorType error21 = CalculateFdError(pli2, pli1, joint_pli.get());
            // 5′ if e(X \ {A} → A) ≤ ε then
            // 6      output X \ {A} → A
            // 7      remove A from C+(X)
            // 8′     if X \ {A} → A holds exactly then
            // 9′         remove all B in R \ X from C+(X)
            // "Holds exactly" means error == 0.0 That is, error == 0.0 for an A on line 4 means
            // only the one column in X \ {A}, which is the single LHS column in the FD that is
            // output, remains in C^+(X).

            // error <= max_fd_error_ means the corresponing RHS column should be deleted (line 7).
            // The RHS column of one is the LHS column of the other.
            // The intersection is what makes it to the final C^+(X).
            auto reg12 = [&]() {
                RegisterAfd(AFD(schema->GetVertical(std::move(CreateEmptyColumnMask().set(col1))),
                                *schema->GetColumn(col2), error12,
                                relation_->GetSharedPtrSchema()));
            };
            auto reg21 = [&]() {
                RegisterAfd(AFD(schema->GetVertical(std::move(CreateEmptyColumnMask().set(col2))),
                                *schema->GetColumn(col1), error21,
                                relation_->GetSharedPtrSchema()));
            };
            // Create the column combination that is the element of L_2, calculate C^+ of it, add
            // them and cache the intersected PLI.
            auto add_to_level = [&](boost::dynamic_bitset<>&& rhs_candidates) {
                current_level.try_emplace(std::move(CreateEmptyColumnMask().set(col1).set(col2)),
                                          std::move(rhs_candidates), std::move(joint_pli));
            };
            if (error12 == 0.0) {
                reg12();
                // Leave only col1.
                if (error21 <= max_fd_error_) {
                    reg21();
                    // Delete col1 too, so we get an empty intersection, which we never add in the
                    // first place instead of deleting later.
                    // current_level.try_emplace(nothing);
                    continue;
                }
                add_to_level(std::move(CreateEmptyColumnMask().set(col1)));
                continue;
            }
            if (error21 == 0.0) {
                reg21();
                // Leave only col2.
                if (error12 <= max_fd_error_) {
                    reg12();
                    // Delete col2 too.
                    continue;
                }
                add_to_level(std::move(CreateEmptyColumnMask().set(col2)));
                continue;
            }
            if (error12 <= max_fd_error_) {
                reg12();
                // Delete col2, everything else remains.
                auto rhs_candidates = not_exact_zeroary_afd_rhss;
                rhs_candidates.reset(col2);
                assert(rhs_candidates.test(col1));
                if (error21 <= max_fd_error_) {
                    reg21();
                    // Also delete col1.
                    rhs_candidates.reset(col1);
                    // TODO: is this possible?
                    if (rhs_candidates.none()) continue;
                }
                add_to_level(std::move(rhs_candidates));
                continue;
            }
            if (error21 <= max_fd_error_) {
                reg21();
                // Delete col1, everything else remains.
                auto rhs_candidates = not_exact_zeroary_afd_rhss;
                rhs_candidates.reset(col1);
                assert(rhs_candidates.test(col2));
                add_to_level(std::move(rhs_candidates));
                continue;
            }
            // Otherwise, C^+(X) remains R.
            add_to_level(boost::dynamic_bitset(not_exact_zeroary_afd_rhss));
        }
    }
}

void Tane::ComputeDependenciesLevel2NonKeys(
        boost::dynamic_bitset<> const& not_exact_zeroary_afd_rhss,
        std::vector<model::Index> const& non_key_attrs_not_0afd,
        std::vector<model::Index> const& non_key_attrs_0afd,
        LevelAttributeSetsData& current_level) {
    ComputeDependenciesLevel2NonKeysZeroaryAfdRhsPairs(not_exact_zeroary_afd_rhss,
                                                       non_key_attrs_0afd, current_level);

    ComputeDependenciesLevel2NonKeysNotBothZeroaryAfdRhs(
            not_exact_zeroary_afd_rhss, non_key_attrs_not_0afd, non_key_attrs_0afd, current_level);
}

// L_2 := GENERATE_NEXT_LEVEL(L_1)
// L_1 contains single-element sets, so we are using an array of these elements.
// Iterate over indices left over from before.
// PREFIX_BLOCKS(L_1) outputs {L_1}, so lines 3 and 4 iterate over all pairs in L_1, and line 5
// obviously holds for every pair.
// Thus, the next level is just all the pairs, all of C^+(X) are known, we don't have to
// actually execute anything here, this can be done on-the-fly in the COMPUTE_DEPENDENCIES(L_2)
// procedure. We have to process 10 cases.
// COMPUTE_DEPENDENCIES(L_2)
// All of C^+(X) are known, so we skip lines 1 and 2.
auto Tane::ComputeDependenciesLevel2(boost::dynamic_bitset<> const& not_exact_zeroary_afd_rhss,
                                     std::vector<model::Index> const& non_key_attrs_not_0afd,
                                     std::vector<model::Index> const& key_attrs_not_0afd,
                                     std::vector<model::Index> const& non_key_attrs_0afd,
                                     std::vector<model::Index> const& key_attrs_0afd,
                                     boost::dynamic_bitset<>& superkey_rhs_candidates)
        -> LevelAttributeSetsData {
    // TODO: use collection emptiness information better, be more concise.
    LevelAttributeSetsData current_level;

    // Start with calculating all pairs where at least one column is a key.
    ComputeDependenciesLevel2Keys(non_key_attrs_not_0afd, key_attrs_not_0afd, non_key_attrs_0afd,
                                  key_attrs_0afd, superkey_rhs_candidates, current_level);

    ComputeDependenciesLevel2NonKeys(not_exact_zeroary_afd_rhss, non_key_attrs_not_0afd,
                                     non_key_attrs_0afd, current_level);

    return current_level;
}

void Tane::ExecuteInternal() {
    if (relation_->GetNumColumns() < 2) return;
    // The first level is handled and the second one is generated on the fly to avoid copying the
    // already calculated PLIs.

    // COMPUTE_DEPENDENCIES(L_1)
    auto [inexact_zeroary_afd_rhss, not_zeroary_afd_rhss, not_exact_zeroary_afd_rhss] =
            ComputeDependenciesLevel1();
    if (max_lhs_ == 0) return;

    // L_1 is the union of these at this point.
    /* TANE, 5 */
    if (inexact_zeroary_afd_rhss.empty() && not_zeroary_afd_rhss.empty()) return;

    // PRUNE(L_1)
    auto [non_key_attrs_not_0afd, key_attrs_not_0afd, non_key_attrs_0afd, key_attrs_0afd,
          superkey_rhs_candidates] =
            PruneLevel1(inexact_zeroary_afd_rhss, not_zeroary_afd_rhss, not_exact_zeroary_afd_rhss);

    // L_2 = GENERATE_NEXT_LEVEL(L_1) and COMPUTE_DEPENDENCIES(L_2)
    LevelAttributeSetsData current_level = ComputeDependenciesLevel2(
            not_exact_zeroary_afd_rhss, non_key_attrs_not_0afd, key_attrs_not_0afd,
            non_key_attrs_0afd, key_attrs_0afd, superkey_rhs_candidates);

    if (max_lhs_ == 1) return;

    for (unsigned int lhs_size = 2; lhs_size <= max_lhs_; ++lhs_size) {
        Prune(current_level);
        LevelAttributeSetsData prev_level = std::move(current_level);
        current_level = GenerateNextLevel(prev_level);

        // while L_l != ∅
        if (current_level.empty()) break;

        ComputeDependencies(current_level, prev_level);
    }
}

}  // namespace algos
