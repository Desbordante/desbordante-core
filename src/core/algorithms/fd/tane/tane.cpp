#include "core/algorithms/fd/tane/tane.h"

#include "core/algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "core/algorithms/fd/pli_based_fd_algorithm.h"
#include "core/algorithms/fd/tane/enums.h"
#include "core/config/error/option.h"
#include "core/config/error_measure/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/model/table/column_data.h"

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
// guaranteed that the bounds would hold up. Right now, the only requirements on the function
// measuring the error are that it must return 0.0 if the FD is exact and it must not decrease with
// increasing LHS attribute set size.
//
// Also, only data for at most two lattice levels is stored, as only the data from the previous
// level may be required.
//
// Most importantly, the algorithm as described in the paper is incorrect, because it deletes
// superkey nodes too eagerly.
//
// In the case of error == 0.0, this means that once we hit line 6 of PRUNE, we might find that C^+
// for some attribute sets that we need has not been calculated. This shows itself when there is a
// column in C^+(X) \ X of a key (iterated in line 5 of PRUNE) that belongs to another key of
// smaller size, as said key's ancestor would have been deleted when processing a previous level,
// with line 2 in COMPUTE_DEPENDENCIES consequently not calculating the further C^+ sets.
//
// If we look at the definition of C^+, we will see that the check on line 6 of PRUNE effectively
// means that no FDs of the shape key sub one column into the C^+(X) \ X hold.
//
// Thus, there are several ways to deal with this: we can perform a direct check on whether an FD
// like this does not hold if we can't find a C^+ set that proves this, we can calculate these sets
// on demand, or we can calculate all the C^+ sets that may be required.
//
// This implementation uses the last way. It extends the calculation process in lines 1 and 2 in
// COMPUTE_DEPENDENCIES with attribute sets that have been detected to be keys. Conceptually,
// superkeys are marked and the marked attribute sets then get skipped before line 4 of
// COMPUTE_DEPENDENCIES and line 4 of PRUNE to simulate deletion, with the mark being spread to all
// the supersets in GENERATE_NEXT_LEVEL's lines 5 and 6.
//
// The C^+ sets can then be computed on the fly in GENERATE_NEXT_LEVEL when looking through the
// subsets in line 5 instead of lines 1 and 2 of COMPUTE_DEPENDENCIES. Notice that when a PLI for an
// attribute set X is accessed in line 5' of COMPUTE_DEPENDENCIES, we had to have accessed the
// C^+(X) set too, and since PLIs are "computed when X is added to its level on line 6 of
// GENERATE_NEXT_LEVEL" and we're computing C^+ sets in this procedure as well, we can store the PLI
// and the C^+ set together. Also note that when we have processed a superkey, we mark it and never
// have to execute line 5' of COMPUTE_DEPENDENCIES for it. Therefore, we can implement the mark as a
// special PLI value. In this case, since we're using a pointer, and that value is nullptr.
//
// Also, with the current implementation it is possible that only values corresponding to superkeys
// are left in the map. In that case, no new FDs are going to be discovered, as they will be skipped
// in COMPUTE_DEPENDENCIES and PRUNE will skip superkeys that are not keys. This implementation
// detects this situation and stops the algorithm in this case. It's not that big of an
// optimization, as this situation would have been detected during next level construction
// otherwise.
//
// Because C^+(X) sets are stored together with X, they are calculated in GENERATE_NEXT_LEVEL, which
// is equivalent to what is described in the article, as COMPUTE_DEPENDENCIES follows
// GENERATE_NEXT_LEVEL if the loop condition holds (TANE lines 6, 8, 5 respectively). If it doesn't,
// then COMPUTE_DEPENDENCIES doesn't get executed due to the level being empty, and the same happens
// here. In the situation where we check every bit in line 5 of GENERATE_NEXT_LEVEL, but the
// attribute set with the last one excluded is not present, we have done useless intersection work,
// but I don't think it matters that much.
//
// In addition to that, the check in lines 2 and 3 of PRUNE is merged with the other procedures in
// the algorithm: if C^+(X) becomes empty in GENERATE_NEXT_LEVEL (with the intersection above), it
// would have no effect in COMPUTE_DEPENDENCIES and would be deleted in a later PRUNE. If it becomes
// empty in COMPUTE_DEPENDENCIES it would have no effect and be deleted in PRUNE once again.
//
// Another correctness point is that key pruning, even in this modified form, still cuts too much.
// The idea here is that the error can be high enough that a dependency from non-key to key can
// hold, while that is impossible when the error is 0. Therefore, if the error is not 0, key pruning
// is disabled entirely. This can be implemented with a minimal modification to the implementation
// of GenerateNextLevel.
//
// The absence of C^+(X) can also be dealt with by directly checking each
// X \ {B} -> A (B ∈ X, A ∉ X) dependency when processing a key in PRUNE if no other C^+(X ∪ {A}
// \ {B}) (line 6) that doesn't contain A exists (i. e. no evidence if X \ {B} -> A holds). If we do
// happen upon it, we know that X \ {B} -> A holds by definition of C^+(X) and don't have to check
// that. There would be a PLI available for each X \ {B}, as those had to have been part of the
// previous level for X to end up in the current one. Line 8 of PRUNE would still be there. If this
// is used with the current map that contains PLIs along with C^+(X), the deletion of an attribute
// set would mean deleting both its PLI and C^+(X). There could be some variation on when exactly
// this is done, either by postponing the deletion until the end of PRUNE or by separating PLIs and
// candidates from the level's attribute sets.
// TODO: try implementing the above as well.

Tane::Tane() : PliBasedAFDAlgorithm() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::kErrorOpt(&max_error_));
    RegisterOption(config::kAfdErrorMeasureOpt(&afd_measure_));
}

bool Tane::Prune(LevelAttributeSetsData& level) {
    bool only_superkeys_left = true;

    RelationalSchema const* schema = relation_->GetSchema();
    /* PRUNE, 1 */
    for (auto it = level.begin(); it != level.end();) {
        auto& [attribute_mask, info] = *it;
        assert(info.rhs_candidates.any());
        if (info.IsProcessedSuperkey()) {
            ++it;
            continue;
        }
        if (/* PRUNE, 4 */ !info.pli->AllValuesAreUnique()) {
            ++it;
            only_superkeys_left = false;
            continue;
        }

        boost::dynamic_bitset<> sibling_attr_set_mask = attribute_mask;

        bool no_candidates_left = true;

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
            // info.rhs_candidates - attribute_mask without allocations
            if (attribute_mask.test(rhs_index)) {
                no_candidates_left = false;
                return;
            }
            sibling_attr_set_mask.set(rhs_index);
            /* PRUNE, 6 */
            for (model::Index i = attribute_mask.find_first(); i != boost::dynamic_bitset<>::npos;
                 i = attribute_mask.find_next(i)) {
                sibling_attr_set_mask.reset(i);
                auto sibling_it = level.find(sibling_attr_set_mask);
                sibling_attr_set_mask.set(i);
                if (sibling_it == level.end() ||
                    !sibling_it->second.rhs_candidates.test(rhs_index)) {
                    sibling_attr_set_mask.reset(rhs_index);
                    no_candidates_left = false;
                    return;
                }
            };
            sibling_attr_set_mask.reset(rhs_index);
            /* PRUNE, 7 */
            RegisterAfd(AFD(schema->GetVertical(attribute_mask), *schema->GetColumn(rhs_index),
                            0.0 /* key -> attr is always a plain FD */,
                            relation_->GetSharedPtrSchema()));

            // Would not be a consideration if the nodes from the level were deleted, but since
            // we've found an FD, we need to delete it from the set of FDs that don't hold.
            // Otherwise, we may output incorrect results in the differently-sized keys case.
            info.rhs_candidates.reset(rhs_index);
            // Sanity check: this reset will not affect the procedure for the following sibling
            // keys, because A is inside X' = (X ∪ {A} \ {B}), and if that is a sibling key, the
            // procedure will iterate through C+(X ∪ {A} \ {B}) \ (X ∪ {A} \ {B}) when it is
            // reached, which obviously does not include A.
            // TODO: The FD has 0.0 error, can we also intersect with LHS like in line 8 of
            // COMPUTE_DEPENDENCIES?
        });

        /* PRUNE, 2-3 */
        if (no_candidates_left) {
            assert(info.rhs_candidates.none());
            it = level.erase(it);
        } else {
            assert(info.rhs_candidates.any());
            // Roughly equivalent to line 8 of PRUNE.
            info.MarkProcessedSuperkey();
            ++it;
        }
    }

    return only_superkeys_left;
}

bool Tane::ComputeDependencies(LevelAttributeSetsData& level,
                               LevelAttributeSetsData const& prev_level) {
    RelationalSchema const* schema = relation_->GetSchema();
    bool only_superkeys_left = true;

    /* COMPUTE_DEPENDENCIES, 3 */
    for (auto it = level.begin(); it != level.end();) {
        auto& [attribute_mask, info] = *it;
        auto& [rhs_candidates, pli] = info;
        assert(rhs_candidates.any());
        if (info.IsProcessedSuperkey()) {
            ++it;
            continue;
        }
        assert(attribute_mask.count() > 1);
        boost::dynamic_bitset<> lhs_attribute_mask = attribute_mask;
        // TODO: the measures should be calculated at the same time as the PLIs are being
        // intersected instead of doing a separate pass, i.e. the intersected PLI should be created
        // here on the first iteration. Then it may be used for later iterations if there is a more
        // efficient way.
        // Creating the intersected PLIs here instead of in GenerateNextLevel doesn't make much
        // sense, since the only effect it will have is maybe allowing us to report more FDs before
        // memory runs out, but the peak memory usage would be the same either way.
        /* COMPUTE_DEPENDENCIES, 4 */
        util::ForEachIndex(info.rhs_candidates, [&](model::Index rhs_index) {
            // info.rhs_candidates & attribute_mask without allocations
            if (!attribute_mask.test(rhs_index)) return;
            /* COMPUTE_DEPENDENCIES, 5' */
            lhs_attribute_mask.reset(rhs_index);
            model::PLI const* lhs_pli = prev_level.find(lhs_attribute_mask)->second.pli.get();
            // LHS is a superkey from the previous level, already processed.
            if (lhs_pli == nullptr) return;
            model::PLI const* rhs_pli = relation_->GetColumnData(rhs_index).GetPositionListIndex();
            config::ErrorType error = CalculateFdError(lhs_pli, rhs_pli, pli.get());
            if (error > max_error_) {
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
            // into a peculiarity with the machines we're using to compute: they mostly work with at
            // least 8 bits in parallel.
            // Except, not quite. The difference with the algorithm in the article is that we're
            // pruning empties immediately. If rhs_candidates becomes empty, we may delete it, which
            // could make some hashtable checks for a node's existence in GenerateNextLevel faster,
            // because a node at the hash(new_attr_set) index in the node array might not exist,
            // so we will avoid an equality check. However, I don't think it's going to happen that
            // much effect.
            // TODO: investigate
            /* COMPUTE_DEPENDENCIES, 8'-9' */
            if (error == 0.0) info.rhs_candidates &= attribute_mask;
        });
        /* PRUNE, 2 */
        if (info.rhs_candidates.none()) {
            /* PRUNE, 3 */
            it = level.erase(it);
        } else {
            ++it;
            only_superkeys_left = false;
        }
    }

    return only_superkeys_left;
}

// Exactly PREFIX_BLOCKS but the order of bits is inverted and attribute set data are stored.
auto Tane::SuffixBlocks(LevelAttributeSetsData const& level) -> SuffixMap {
    SuffixMap map;
    for (auto const& [attribute_mask, info] : level) {
        assert(attribute_mask.size() >= 2);
        // Only one attribute set with this suffix is possible, avoid adding.
        // TODO: test if this does anything.
        if (attribute_mask.test(0) && attribute_mask.test(1)) continue;
        boost::dynamic_bitset<> suffix = attribute_mask;
        model::Index const non_suffix_column = suffix.find_first();
        suffix.reset(non_suffix_column);
        map[std::move(suffix)].emplace_back(non_suffix_column, &info);
    }
    return map;
}

auto Tane::GenerateNextLevel(LevelAttributeSetsData const& level) -> LevelAttributeSetsData {
    /* GENERATE_NEXT_LEVEL, 1 */
    LevelAttributeSetsData next_level;
    SuffixMap suffix_map = SuffixBlocks(level);
    /* GENERATE_NEXT_LEVEL, 2 */
    for (auto s_map_it = suffix_map.begin(); s_map_it != suffix_map.end();) {
        SuffixMap::node_type node = suffix_map.extract(s_map_it++);
        std::vector<NoSuffixAttributeSetDataReference>& prev_level_masks = node.mapped();
        if (prev_level_masks.size() < 2) continue;
        boost::dynamic_bitset<>& new_mask = node.key();
        /* GENERATE_NEXT_LEVEL, 3 */
        for (auto outer_it = prev_level_masks.begin(), end_it = std::prev(prev_level_masks.end());
             outer_it != end_it; ++outer_it) {
            assert(outer_it->info->IsProcessedSuperkey() ||
                   !outer_it->info->pli->AllValuesAreUnique());
            /* GENERATE_NEXT_LEVEL, 4 */
            new_mask.set(outer_it->non_suffix_column);
            for (auto inner_it = std::next(outer_it); inner_it != prev_level_masks.end();
                 ++inner_it) {
                /* PRUNE, 2-3 */
                if (!outer_it->info->rhs_candidates.intersects(inner_it->info->rhs_candidates))
                    continue;
                /* GENERATE_NEXT_LEVEL, 4 */
                new_mask.set(inner_it->non_suffix_column);
                /* COMPUTE_DEPENDENCIES, 2 */
                boost::dynamic_bitset<> next_candidates =
                        inner_it->info->rhs_candidates & outer_it->info->rhs_candidates;
                assert(inner_it->info->IsProcessedSuperkey() ||
                       !inner_it->info->pli->AllValuesAreUnique());
                bool children_no_superkeys = !outer_it->info->IsProcessedSuperkey() &&
                                             !inner_it->info->IsProcessedSuperkey();
                bool children_some_non_superkeys = !outer_it->info->IsProcessedSuperkey() ||
                                                   !inner_it->info->IsProcessedSuperkey();

                bool in_next_level = true;
                /* GENERATE_NEXT_LEVEL, 5 */
                // No point in checking anything earlier, we already know they exist because we got
                // them from SuffixBlocks.
                for (model::Index i = new_mask.find_next(inner_it->non_suffix_column);
                     i != boost::dynamic_bitset<>::npos; i = new_mask.find_next(i)) {
                    new_mask.reset(i);
                    auto it = level.find(new_mask);
                    new_mask.set(i);
                    if (it == level.end() ||
                        (/* COMPUTE_DEPENDENCIES, 2 */ next_candidates &= it->second.rhs_candidates)
                                /* PRUNE, 2-3 */.none()) {
                        in_next_level = false;
                        break;
                    }
                    if (it->second.IsProcessedSuperkey()) {
                        children_no_superkeys = false;
                    } else {
                        children_some_non_superkeys = true;
                    }
                }
                if (in_next_level) {
                    std::unique_ptr<model::PLI> pli;
                    if (children_no_superkeys) {
                        pli = outer_it->info->pli->Intersect(inner_it->info->pli.get());
                    } else if (max_error_ != 0.0 && children_some_non_superkeys) {
                        // All superkey PLIs have an empty index, so it doesn't matter which exact
                        // columns have been intersected.
                        pli = model::PLI::MakeSuperkeyPLI(relation_->GetNumRows());
                    } else {
                        // For error != 0.0: if all child attribute sets are superkeys, all of them
                        // will be skipped in ComputeDependencies, avoid extra checks there.
                        // For error == 0.0: all further superkeys are skipped.
                        pli = nullptr;
                    }
                    /* GENERATE_NEXT_LEVEL, 6 */
                    next_level.try_emplace(new_mask, next_candidates, std::move(pli));
                }
                new_mask.reset(inner_it->non_suffix_column);
            }
            new_mask.reset(outer_it->non_suffix_column);
        }
    }
    return next_level;
}

void Tane::ExecuteInternal() {
    if (relation_->GetNumColumns() < 2) return;
    // The first level is handled and the second one is generated on the fly to avoid copying the
    // already calculated PLIs.
    // See tane_initial.cpp for the code.

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

    // L_2 = GENERATE_NEXT_LEVEL(L_1); COMPUTE_DEPENDENCIES(L_2)
    LevelAttributeSetsData current_level = ComputeDependenciesLevel2(
            not_exact_zeroary_afd_rhss, non_key_attrs_not_0afd, key_attrs_not_0afd,
            non_key_attrs_0afd, key_attrs_0afd, superkey_rhs_candidates);

    for (unsigned int lhs_size = 2; lhs_size <= max_lhs_; ++lhs_size) {
        bool only_superkeys_left = Prune(current_level);
        if (only_superkeys_left) break;
        LevelAttributeSetsData prev_level = std::move(current_level);
        current_level = GenerateNextLevel(prev_level);

        // while L_l != ∅
        if (current_level.empty()) break;

        only_superkeys_left = ComputeDependencies(current_level, prev_level);
        if (only_superkeys_left) break;
    }
}

void Tane::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::kErrorOpt.GetName(), config::names::kAfdErrorMeasure});
}

config::ErrorType Tane::CalculateZeroAryFdError(ColumnData const* rhs) {
    // NOTE: Sometimes the empty LHS case is not defined, so we have to figure out a value that
    // makes sense on our own. If the RHS is constant, there is an FD, so it only makes sense for
    // error to be 0.
    switch (afd_measure_) {
        case algos::AfdErrorMeasure::kPerValue:
        case algos::AfdErrorMeasure::kG3: {
            std::size_t max = 1;
            model::PositionListIndex const* x_pli = rhs->GetPositionListIndex();
            for (model::PLI::Cluster const& x_cluster : x_pli->GetIndex()) {
                std::size_t const x_cluster_size = x_cluster.size();
                if (max < x_cluster_size) max = x_cluster_size;
            }
            return 1.0 - static_cast<config::ErrorType>(max) / x_pli->GetRelationSize();
        }
        case algos::AfdErrorMeasure::kG1:
            return afd_metric_calculator::AFDMetricCalculator::CalculateZeroAryG1(
                    rhs, relation_.get()->GetNumTuplePairs());
        /*
         * dom_{empty_set}(R) needs some care in its definition. If that care is taken, we get
         * |dom_{empty_set}(R)| = 1.
         */
        case algos::AfdErrorMeasure::kRho:
            return static_cast<config::ErrorType>(rhs->GetPositionListIndex()->GetNumCluster() -
                                                  1) /
                   rhs->GetPositionListIndex()->GetNumCluster();
        /*
         * The original definition of this one requires the presence of two attributes. The exact
         * expression used is pdep(X, Y) = p(R1.Y = R.Y2 | R1.X = R2.X). If we treat the projection
         * of a tuple on empty X as the empty set, then we get pdep({}, Y) = p(R1.Y = R.Y2), which
         * is exactly the self-dependency measure pdep(Y).
         */
        case algos::AfdErrorMeasure::kPdep:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculatePdepSelf(
                               rhs->GetPositionListIndex());
        /*
         * The probability that a tuple participates in a violating pair is 0 if there is an FD,
         * otherwise it is 1 for an empty LHS and non-constant RHS.
         */
        case algos::AfdErrorMeasure::kG2:
        /*
         * For an empty LHS, the mutual information is 0, but if the entropy of RHS is also 0 (i.e.
         * it is constant), the measure is technically undefined.
         */
        case algos::AfdErrorMeasure::kFi:
        /*
         * When using pdep({}, Y) = pdep(Y), tau has 0 in the numerator. If pdep(Y) = 0, it has 0 in
         * the denominator too, so it is technically undefined.
         */
        case algos::AfdErrorMeasure::kTau:
        /*
         * Since Y is taken to be constant in the definition, pdep({} -> Y, R) is just pdep(Y). The
         * expected value of a constant is that constant, so we get a 0 in the numerator. The
         * denominator is again 0 when the RHS is constant, so this measure is technically undefined
         * as well in that case.
         */
        case algos::AfdErrorMeasure::kMuPlus:
            return rhs->GetPositionListIndex()->IsConstant() ? 0.0 : 1.0;
    }
    assert(false);
    __builtin_unreachable();
}

config::ErrorType Tane::CalculateFdError(model::PLI const* lhs_pli, model::PLI const* rhs_pli,
                                         model::PLI const* joint_pli) {
    switch (afd_measure_) {
        case algos::AfdErrorMeasure::kPdep:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculatePdepMeasure(lhs_pli,
                                                                                        rhs_pli);
        case algos::AfdErrorMeasure::kTau:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateTau(lhs_pli, rhs_pli);
        case algos::AfdErrorMeasure::kMuPlus:
            return 1 -
                   afd_metric_calculator::AFDMetricCalculator::CalculateMuPlus(lhs_pli, rhs_pli);
        case algos::AfdErrorMeasure::kRho:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateRhoMeasure(lhs_pli,
                                                                                       joint_pli);
        case algos::AfdErrorMeasure::kFi:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateFI(
                               lhs_pli, rhs_pli, relation_.get()->GetNumRows());
        case algos::AfdErrorMeasure::kG2:
            return afd_metric_calculator::AFDMetricCalculator::CalculateG2Error(
                    lhs_pli, rhs_pli, relation_.get()->GetNumRows());
        case algos::AfdErrorMeasure::kG3:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculateG3(
                               lhs_pli, rhs_pli, relation_.get()->GetNumRows());
        case algos::AfdErrorMeasure::kG1:
            return afd_metric_calculator::AFDMetricCalculator::CalculateG1Error(
                    lhs_pli, joint_pli, relation_.get()->GetNumTuplePairs());
        case algos::AfdErrorMeasure::kPerValue:
            return 1 - afd_metric_calculator::AFDMetricCalculator::CalculatePerValue(lhs_pli,
                                                                                     joint_pli);
    }
    assert(false);
    __builtin_unreachable();
}

}  // namespace algos
