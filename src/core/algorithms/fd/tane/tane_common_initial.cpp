#include "core/algorithms/fd/tane/tane_common.h"

// The code for the initial stages of TANE as implemented here is long and involved, so it is
// separated into this file for readability of the main file.

namespace algos::tane {
// COMPUTE_DEPENDENCIES(L_1)
// Lines 1 and 2 are pointless, C^+({A}) is exactly R at this point
// Lines 3 and 4 reduce to "for each attribute"
// Line 5 is an error calculation for an FD with an empty LHS.
// If the condition on line 8' holds, the attribute set will be pruned (C^+({A}) = ∅), which is
// exactly the same as if there was no such attribute set in the first place, so we can just not add
// it.
auto TaneCommon::ComputeDependenciesLevel1() -> FirstLevelComputeDependenciesResult {
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
auto TaneCommon::PruneLevel1(std::vector<model::Index> const& inexact_zeroary_afd_rhss,
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
        if (!relation_->GetColumnData(not_zeroary_afd_rhs)
                     .GetPositionListIndex()
                     ->AllValuesAreUnique()) {
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
        if (!relation_->GetColumnData(inexact_zeroary_afd_rhs)
                     .GetPositionListIndex()
                     ->AllValuesAreUnique()) {
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
void TaneCommon::ComputeDependenciesLevel2KeysNotZeroaryAfdRhs(
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
void TaneCommon::ComputeDependenciesLevel2KeysZeroaryAfdRhs(
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
void TaneCommon::ComputeDependenciesLevel2Keys(
        std::vector<model::Index> const& non_key_attrs_not_0afd,
        std::vector<model::Index> const& key_attrs_not_0afd,
        std::vector<model::Index> const& non_key_attrs_0afd,
        std::vector<model::Index> const& key_attrs_0afd,
        boost::dynamic_bitset<>& superkey_rhs_candidates, LevelAttributeSetsData& current_level) {
    ComputeDependenciesLevel2KeysNotZeroaryAfdRhs(non_key_attrs_not_0afd, key_attrs_not_0afd,
                                                  non_key_attrs_0afd, key_attrs_0afd,
                                                  superkey_rhs_candidates, current_level);
    ComputeDependenciesLevel2KeysZeroaryAfdRhs(non_key_attrs_not_0afd, non_key_attrs_0afd,
                                               key_attrs_0afd, superkey_rhs_candidates,
                                               current_level);
}

// Case 8: non_key_attrs_0afd, non_key_attrs_0afd
// C^+(X) = (R \ {A}) ⋂ (R \ {B}) = R \ {A, B}
void TaneCommon::ComputeDependenciesLevel2NonKeysZeroaryAfdRhsPairs(
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
            current_level.try_emplace(
                    std::move(CreateEmptyColumnMask().set(col1).set(col2)),
                    std::move(boost::dynamic_bitset(not_exact_zeroary_afd_rhss)
                                      .reset(col1)
                                      .reset(col2)),
                    relation_->GetColumnData(col1).GetPositionListIndex()->Intersect(
                            relation_->GetColumnData(col2).GetPositionListIndex()));
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
void TaneCommon::ComputeDependenciesLevel2NonKeysNotBothZeroaryAfdRhs(
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
            model::PLI const* pli1 = relation_->GetColumnData(col1).GetPositionListIndex();
            model::PLI const* pli2 = relation_->GetColumnData(col2).GetPositionListIndex();
            std::unique_ptr<model::PLI> joint_pli = pli1->Intersect(pli2);

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
            model::PLI const* pli1 = relation_->GetColumnData(col1).GetPositionListIndex();
            model::PLI const* pli2 = relation_->GetColumnData(col2).GetPositionListIndex();
            std::unique_ptr<model::PLI> joint_pli = pli1->Intersect(pli2);

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

            // error <= max_fd_error_ means the corresponding RHS column should be deleted (line 7).
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

void TaneCommon::ComputeDependenciesLevel2NonKeys(
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
auto TaneCommon::ComputeDependenciesLevel2(
        boost::dynamic_bitset<> const& not_exact_zeroary_afd_rhss,
        std::vector<model::Index> const& non_key_attrs_not_0afd,
        std::vector<model::Index> const& key_attrs_not_0afd,
        std::vector<model::Index> const& non_key_attrs_0afd,
        std::vector<model::Index> const& key_attrs_0afd,
        boost::dynamic_bitset<>& superkey_rhs_candidates) -> LevelAttributeSetsData {
    // TODO: use collection emptiness information better, be more concise.
    LevelAttributeSetsData current_level;

    // Start with calculating all pairs where at least one column is a key.
    ComputeDependenciesLevel2Keys(non_key_attrs_not_0afd, key_attrs_not_0afd, non_key_attrs_0afd,
                                  key_attrs_0afd, superkey_rhs_candidates, current_level);

    ComputeDependenciesLevel2NonKeys(not_exact_zeroary_afd_rhss, non_key_attrs_not_0afd,
                                     non_key_attrs_0afd, current_level);

    return current_level;
}
}  // namespace algos::tane
