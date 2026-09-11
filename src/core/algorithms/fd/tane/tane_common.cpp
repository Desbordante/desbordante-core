#include "core/algorithms/fd/tane/tane_common.h"

#include <iomanip>
#include <list>
#include <memory>

#include "core/config/error/option.h"
#include "core/model/table/column_data.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/relational_schema.h"
#include "core/util/logger.h"

namespace algos::tane {
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
// Also, only data for at most two lattice levels is stored, as only the data from the previous
// level may be required.
//
// Most importantly, the algorithm as described in the paper is incorrect, because it deletes
// superkey nodes too eagerly, leading to failures when the intersection on line 6 of PRUNE is
// checked, due to absence of some C^+ sets, as we might not have calculated them. This shows itself
// when there is a column in C^+(X) \ X of a key (iterated in line 5 of PRUNE) that belongs to
// another key of smaller size, as said key's ancestor would have been deleted when processing a
// previous level, with line 2 in COMPUTE_DEPENDENCIES consequently not calculating the further C^+
// sets.
//
// There are at least two ways to deal with this. This implementation chooses to not delete key
// attribute sets (line 8 of PRUNE) from the level but instead marks them as superkeys. The marked
// attribute sets then get skipped, conceptually, after a check for the mark that would be after
// line 3 of COMPUTE_DEPENDENCIES, while lines 1 and 2 are executed normally, and before line 4 of
// PRUNE. The mark is spread in GENERATE_NEXT_LEVEL, where if one direct subset (line 5) has it, the
// attribute set on the new level has it as well. Metanome's implementation of TANE does this too.
//
// When implemented this way, it makes sense to store all C^+(X) together with X in a single
// map, along with the corresponding PLI. The mark doesn't have to be stored, because PLIs are only
// needed for error calculations, and they are never performed when the attribute set is a superkey.
// Therefore, we can use the absence of a PLI as the superkey mark.
//
// Also, with the current implementation it is possible that only values corresponding to superkeys
// are left in the map. In that case, no new FDs are going to be discovered, as they will be skipped
// in COMPUTE_DEPENDENCIES and PRUNE will skip superkeys that are not keys. This implementation
// detects this situation and stops the algorithm in this case. It's not that big of an
// optimization, as this situation would have been detected during next level construction
// otherwise.
//
// The absence of C^+(X) can also be dealt with by directly checking each
// X \ {B} -> A (B ∈ X, A ∉ X) dependency when processing a key in PRUNE if no other C^+(X ∪ {A}
// \ {B}) (line 6) that doesn't contain A exists (i. e. no evidence if X \ {B} -> A holds). If we do
// happen upon it, we know that X \ {B} -> A holds by definition of C^+(X) and don't have to check
// that. There would be a PLI available for each X \ {B}, as those had to have been part of the
// previous level for X to end up in the current one. Line 8 would still be there. If this is used
// with the current map that contains PLIs along with C^+(X), the deletion of an attribute set would
// mean deleting both its PLI and C^+(X). There could be some variation on when exactly this is
// done, either by postponing the deletion until the end of PRUNE or by separating PLIs and
// candidates from the level's attribute sets.
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
// In addition to that, the check in lines 2 and 3 of PRUNE is merged with the other procedures in
// the algorithm: if C^+(X) becomes empty in GENERATE_NEXT_LEVEL (with the intersection above), it
// would have no effect in COMPUTE_DEPENDENCIES and would be deleted in a later PRUNE. If it becomes
// empty in COMPUTE_DEPENDENCIES it would have no effect and be deleted in PRUNE once again.

TaneCommon::TaneCommon() : PliBasedAFDAlgorithm() {
    RegisterOption(config::kErrorOpt(&max_fd_error_));
}

bool TaneCommon::Prune(LevelAttributeSetsData& level) {
    bool only_superkeys_left = true;

    RelationalSchema const* schema = relation_->GetSchema();
    /* PRUNE, 1 */
    for (auto it = level.begin(); it != level.end();) {
        auto& [column_combination, info] = *it;
        assert(info.rhs_candidates.any());
        if (info.IsSuperkey()) {
            ++it;
            continue;
        }
        if (/* PRUNE, 4 */ !info.pli->AllValuesAreUnique()) {
            ++it;
            only_superkeys_left = false;
            continue;
        }
        // Only executed if we have a key, superkeys have already been processed.

        boost::dynamic_bitset<> sibling = column_combination;

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
            // info.rhs_candidates - column_combination without allocations
            if (column_combination.test(rhs_index)) {
                no_candidates_left = false;
                return;
            }
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
                    no_candidates_left = false;
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

        /* PRUNE, 2-3 */
        if (no_candidates_left) {
            assert(info.rhs_candidates.none());
            it = level.erase(it);
        } else {
            assert(info.rhs_candidates.any());
            // Roughly equivalent to line 8 of PRUNE.
            info.MarkSuperkey();
            ++it;
        }
    }

    return only_superkeys_left;
}

bool TaneCommon::ComputeDependencies(LevelAttributeSetsData& level,
                                     LevelAttributeSetsData const& prev_level) {
    RelationalSchema const* schema = relation_->GetSchema();
    bool only_superkeys_left = true;

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
            model::PLI const* lhs_pli = prev_level.find(lhs_attribute_mask)->second.pli.get();
            model::PLI const* rhs_pli = relation_->GetColumnData(rhs_index).GetPositionListIndex();
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
            only_superkeys_left = false;
        }
    }

    return only_superkeys_left;
}

// Exactly PREFIX_BLOCKS but the order of bits is inverted and RHS candidates are stored.
auto TaneCommon::SuffixBlocks(LevelAttributeSetsData const& level) -> SuffixMap {
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

auto TaneCommon::GenerateNextLevel(LevelAttributeSetsData const& level) -> LevelAttributeSetsData {
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
                    std::unique_ptr<model::PLI> pli =
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

void TaneCommon::ExecuteInternal() {
    if (relation_->GetNumColumns() < 2) return;
    // The first level is handled and the second one is generated on the fly to avoid copying the
    // already calculated PLIs.
    // See tane_common_initial.cpp for the code.

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

}  // namespace algos::tane
