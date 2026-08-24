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

void Tane::Prune(Level& level) {
    RelationalSchema const* schema = relation_->GetSchema();

    for (auto it = level.begin(); it != level.end();) {
        auto& [column_combination, metadata] = *it;
        assert(metadata.rhs_candidates.any());
        // Lines 2 and 3 have been done in GenerateNextLevel and ComputeDependencies
        // TODO: collect proactively?
        if (!IsKey(metadata.position_list_index.get())) {
            ++it;
            continue;
        }
        boost::dynamic_bitset<> sibling_scratch = column_combination;
        util::ForEachIndex(
                metadata.rhs_candidates - column_combination, [&](model::Index rhs_index) {
                    sibling_scratch.set(rhs_index);
                    bool not_candidate_for_all = false;
                    for (model::Index j = column_combination.find_first();
                         j != boost::dynamic_bitset<>::npos; j = column_combination.find_next(j)) {
                        sibling_scratch.reset(j);
                        auto it = level.find(sibling_scratch);
                        sibling_scratch.set(j);
                        if (it == level.end() || !it->second.rhs_candidates.test(rhs_index)) {
                            not_candidate_for_all = true;
                            break;
                        }
                    }
                    sibling_scratch.reset(rhs_index);
                    if (not_candidate_for_all) return;
                    RegisterAfd(AFD(schema->GetVertical(column_combination),
                                    *schema->GetColumn(rhs_index),
                                    0.0 /* key -> attr is always a plain FD */,
                                    relation_->GetSharedPtrSchema()));
                    // make sibling keys ignore this one
                    metadata.rhs_candidates.reset(rhs_index);
                });
        // it = level.erase(it);
    }
    // Can't delete immediately due to sibling keys.
    for (auto it = level.begin(); it != level.end();) {
        if (IsKey(it->second.position_list_index.get())) {
            it = level.erase(it);
        } else {
            ++it;
        }
    }
}

void Tane::ComputeDependencies(Level& current_level, Level const& prev_level) {
    RelationalSchema const* schema = relation_->GetSchema();

    // Lines 1 and 2 have been done in GenerateNextLevel

    for (auto it = current_level.begin(); it != current_level.end();) {
        auto& [column_combination, metadata] = *it;
        assert(metadata.position_list_index == nullptr);
        boost::dynamic_bitset<> parent_lhs = column_combination;
        model::Index const column = parent_lhs.find_first();
        parent_lhs.reset(column);
        metadata.position_list_index =
                prev_level.find(parent_lhs)
                        ->second.position_list_index->Intersect(
                                relation_->GetColumnData(column).GetPLWSIndex());
        parent_lhs.set(column);
        util::ForEachIndex(
                column_combination & metadata.rhs_candidates, [&](model::Index rhs_index) {
                    parent_lhs.reset(rhs_index);
                    assert(prev_level.contains(parent_lhs));
                    model::PLIWS const* pli =
                            prev_level.find(parent_lhs)->second.position_list_index.get();
                    model::PLIWS const* rhs_pli =
                            relation_->GetColumnData(rhs_index).GetPLWSIndex();

                    config::ErrorType error =
                            CalculateFdError(pli, rhs_pli, metadata.position_list_index.get());
                    if (error > max_fd_error_) {
                        parent_lhs.set(rhs_index);
                        return;
                    }
                    // if e(X \ {A} → A) ≤ ε then

                    // output X \ {A} → A
                    RegisterAfd(AFD(schema->GetVertical(parent_lhs), *schema->GetColumn(rhs_index),
                                    error, relation_->GetSharedPtrSchema()));
                    parent_lhs.set(rhs_index);

                    // remove A from C^+(X)
                    metadata.rhs_candidates.reset(rhs_index);

                    if (error != 0) return;
                    // if X \ {A} → A holds exactly then

                    // remove all B in R \ X from C^+(X)
                    metadata.rhs_candidates &= parent_lhs;
                });
        if (metadata.rhs_candidates.none()) {
            it = current_level.erase(it);
        } else {
            ++it;
        }
    }
}

// Exactly PrefixBlocks but the order of bits is inverted, plus we store C^+(X).
auto Tane::SuffixBlocks(Level const& level) -> SuffixMap {
    SuffixMap map;
    for (auto const& [column_combination, metadata] : level) {
        boost::dynamic_bitset<> suffix = column_combination;
        model::Index const non_suffix_column = suffix.find_first();
        suffix.reset(non_suffix_column);
        map[std::move(suffix)].emplace_back(non_suffix_column, &metadata.rhs_candidates);
    }
    return map;
}

auto Tane::GenerateNextLevel(Level& current_level) -> Level {
    Level next_level;
    SuffixMap s_map = SuffixBlocks(current_level);
    for (auto s_map_it = s_map.begin(); s_map_it != s_map.end();) {
        SuffixMap::node_type node = s_map.extract(s_map_it++);
        std::vector<PrevLevelColumnCombinationInfo>& infos = node.mapped();
        std::ranges::sort(infos, [](PrevLevelColumnCombinationInfo const& i1,
                                    PrevLevelColumnCombinationInfo const& i2) {
            return i1.non_suffix_column < i2.non_suffix_column;
        });
        assert(!infos.empty());
        boost::dynamic_bitset<>& column_combination = node.key();
        for (auto outer_info_it = infos.begin(), end_it = std::prev(infos.end());
             outer_info_it != end_it; ++outer_info_it) {
            auto const [outer_cand_index, outer_rhs_candidates] = *outer_info_it;
            column_combination.set(outer_cand_index);
            for (auto inner_info_it = std::next(outer_info_it); inner_info_it != infos.end();
                 ++inner_info_it) {
                auto const [inner_cand_index, inner_rhs_candidates] = *inner_info_it;
                if (!outer_rhs_candidates->intersects(*inner_rhs_candidates)) continue;
                boost::dynamic_bitset<> new_rhs_candidates =
                        *outer_rhs_candidates & *inner_rhs_candidates;

                column_combination.set(inner_cand_index);
                bool add = true;
                // outer + suffix and inner + suffix have been encountered in SuffixBlocks already,
                // no point in checking for their existence.
                for (model::Index removed_column = column_combination.find_next(inner_cand_index);
                     removed_column != boost::dynamic_bitset<>::npos;
                     removed_column = column_combination.find_next(removed_column)) {
                    column_combination.reset(removed_column);
                    auto it = current_level.find(column_combination);
                    column_combination.set(removed_column);
                    if (it == current_level.end() ||
                        (new_rhs_candidates &= it->second.rhs_candidates).none()) {
                        add = false;
                        break;
                    }
                }
                if (add) {
                    next_level.try_emplace(column_combination, std::move(new_rhs_candidates));
                }
                column_combination.reset(inner_cand_index);
            }
            column_combination.reset(outer_cand_index);
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
    // COMPUTE_DEPENDENCIES(L_1)
    // (1) for each X ∈ L_1 do (2) C^+(X) := ⋂A∈X C+(X \ {A})
    // pointless, C^+({A}) is exactly R
    // (3) and (4) reduce to "for each attribute"
    // if ∅ → A is valid then
    //   output ∅ → A
    //   C^+({A}) = ∅ <- lines 7 and 8 reduce to this
    // Upon encountering C^+(X) = ∅, PRUNE will delete the child node, which is exactly the
    // same as if there was no such node in the first place.
    // We could remove zeroary LHS's RHSs, but (our) GenerateNextLevel will perform the initial
    // intersections from COMPUTE_DEPENDENCIES and the deletion from lines 2 and 3 of PRUNE
    // immediately, so they are going to be accounted for on the next level.

    RelationalSchema const* schema = relation_->GetSchema();
    std::vector<model::Index> not_zeroary_afd_rhs;
    for (model::Index column_index = 0; column_index != relation_->GetNumColumns();
         ++column_index) {
        ColumnData const& column_data = relation_->GetColumnData(column_index);
        double fd_error = CalculateZeroAryFdError(&column_data);
        if (fd_error <= max_fd_error_) {
            // if X \ {A} → A is valid
            // output X \ {A} → A
            RegisterAfd(AFD(schema->CreateEmptyVertical(), *schema->GetColumn(column_index),
                            fd_error, relation_->GetSharedPtrSchema()));
            // remove A from C^+({A})
            // remove all B in R \ {A} from C^+({A})
            // C^+({A}) = ∅
            continue;
        }
        // C^+({A}) = R
        not_zeroary_afd_rhs.push_back(column_index);
    }
    if (max_lhs_ == 0) return;
    // PRUNE(L_1) will not have anything to iterate over in lines 5 and 6 (see below), so it will
    // either just delete the node or not. GENERATE_NEXT_LEVEL(L_1) will output the empty set either
    // way.
    if (not_zeroary_afd_rhs.size() <= 1) return;

    // PRUNE(L_1)
    // Lines 2 and 3 can be omitted as attributes with C^+({A}) = ∅ have been deleted.
    // Line 4: keep as is
    // C^+({D}) \ {D} is R \ {D} (or C^+({A}) = ∅, which is deleted)
    // Line 5 is iteration over all attributes except the one in X (noop for the ∅ case)
    // C^+({D} ∪ {A} \ {D}) = C^+({A}), which is either ∅ or R.
    // It is ∅ for valid RHSs (determined in COMPUTE_DEPENDENCIES(L_1)), and R otherwise.
    // Line 6 condition on L_1 is false for valid RHSs, true otherwise
    // So lines 5 and 6 mean iteration over all attributes that are not valid RHSs and not the
    // element in X
    std::vector<model::Index> non_key_attrs;
    for (auto lhs_it = not_zeroary_afd_rhs.begin(); lhs_it != not_zeroary_afd_rhs.end();) {
        model::Index const key_column = *lhs_it;
        if (!IsKey(relation_->GetColumnData(key_column).GetPositionListIndex())) {
            non_key_attrs.push_back(key_column);
            ++lhs_it;
            continue;
        }
        // This column only consists of unique values, so any FD with it as LHS holds exactly.
        for (auto rhs_it = not_zeroary_afd_rhs.begin(); rhs_it != lhs_it; ++rhs_it) {
            RegisterAfd(
                    AFD(schema->GetVertical(std::move(
                                boost::dynamic_bitset<>(schema->GetNumColumns()).set(key_column))),
                        *schema->GetColumn(*rhs_it), 0.0, relation_->GetSharedPtrSchema()));
        }
        for (auto rhs_it = ++lhs_it; rhs_it != not_zeroary_afd_rhs.end(); ++rhs_it) {
            RegisterAfd(
                    AFD(schema->GetVertical(std::move(
                                boost::dynamic_bitset<>(schema->GetNumColumns()).set(key_column))),
                        *schema->GetColumn(*rhs_it), 0.0, relation_->GetSharedPtrSchema()));
        }
    }
    // GENERATE_NEXT_LEVEL(L_1) will output the empty set (nothing to iterate over).
    if (non_key_attrs.size() <= 1) return;

    // L_2 := GENERATE_NEXT_LEVEL(L_1)
    // L_1 contains single-element sets, so we can use an array of these elements.
    // Iterate over indices left over from before.
    // PREFIX_BLOCKS(L_1) outputs L_1, so lines 3 and 4 iterate over all pairs in L_1, and line 5
    // obviously holds for every pair.
    // Thus, the next level is just all the pairs, all of C^+(X) = R, we
    // don't have to actually execute anything here, this can be done on-the-fly in the
    // COMPUTE_DEPENDENCIES(L_2) procedure

    // COMPUTE_DEPENDENCIES(L_2)
    Level current_level;
    // Exactly 2 bits set in a column combination at this point, not less than that later on,PLIs
    // are all newly intersected.
    // All of X have C^+(X) = R, since the previous level had C^+(X) = R for all the nodes left
    // over, so we skip lines 1 and 2.
    for (auto col1_it = non_key_attrs.begin(), end_it = std::prev(non_key_attrs.end());
         col1_it != end_it; ++col1_it) {
        model::Index const col1 = *col1_it;
        for (auto col2_it = std::next(col1_it); col2_it != non_key_attrs.end(); ++col2_it) {
            // for each A ∈ X ∩ C+(X) <=> for each A ∈ X ∩ R <=> for each A ∈ X
            model::Index const col2 = *col2_it;

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

            // This stuff is annoying to read in full, abbreviating.
            // Allocate the bitset with the number of columns bits.
            auto bs = [&]() { return boost::dynamic_bitset<>(relation_->GetNumColumns()); };
            auto reg12 = [&]() {
                RegisterAfd(AFD(schema->GetVertical(std::move(bs().set(col1))),
                                *schema->GetColumn(col2), error12,
                                relation_->GetSharedPtrSchema()));
            };
            auto reg21 = [&]() {
                RegisterAfd(AFD(schema->GetVertical(std::move(bs().set(col2))),
                                *schema->GetColumn(col1), error21,
                                relation_->GetSharedPtrSchema()));
            };
            // Create the column combination that is the element of L_2, calculate C^+ of it, add
            // them and cache the intersected PLI.
            auto add_to_level = [&](boost::dynamic_bitset<>&& rhs_candidates) {
                current_level.try_emplace(std::move(bs().set(col1).set(col2)),
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
                add_to_level(std::move(bs().set(col1)));
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
                add_to_level(std::move(bs().set(col2)));
                continue;
            }
            if (error12 <= max_fd_error_) {
                reg12();
                // Delete col2, everything else remains.
                auto rhs_candidates = bs();
                rhs_candidates.set().reset(col2);
                if (error21 <= max_fd_error_) {
                    reg21();
                    // Also delete col1.
                    rhs_candidates.reset(col1);
                }
                add_to_level(std::move(rhs_candidates));
                continue;
            }
            if (error21 <= max_fd_error_) {
                reg21();
                // Delete col1, everything else remains.
                add_to_level(std::move(bs().set().reset(col1)));
                continue;
            }
            // Otherwise, C^+(X) is R.
            add_to_level(std::move(bs().set()));
        }
    }

    if (max_lhs_ == 1) return;

    // TODO: figure out what happens when LHS size reaches the maximum.
    for (unsigned int lhs_size = 2; lhs_size <= max_lhs_; ++lhs_size) {
        Prune(current_level);
        Level prev_level = std::move(current_level);
        current_level = GenerateNextLevel(prev_level);

        // while L_l != ∅
        if (current_level.empty()) break;

        ComputeDependencies(current_level, prev_level);
    }
}

}  // namespace algos
