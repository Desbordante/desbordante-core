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
// The original TANE algorithm is incorrect, because it deletes superkey nodes too early, leading to
// failures when the intersection on line 6 of PRUNE is checked, as we might not have one or more of
// the intersected C^+ sets calculated.
// This shows itself when there is a column in C^+(X) \ X of a key (line 5 of PRUNE) that belongs to
// another key of smaller size, as it would be deleted when processing a previous level, with line 2
// in COMPUTE_DEPENDENCIES consequently not calculating the further C^+ sets.
// One solution is to directly check each X \ {B} -> A (B ∈ X, A ∉ X) dependency when encountering a
// key if no other C^+(X ∪ {A} \ {B}) that doesn't contain A exists (i. e. no evidence if
// X \ {B} -> A holds). If we do happen upon it, we know that X \ {B} -> A holds and no direct
// checking is needed. There would be a PLI available for each X \ {B}, as those had to have been
// part of the previous level for X to end up in the current one.
// Another is to, instead of deleting the key nodes, keep them and mark them, then skip the marked
// ones in line 3 of COMPUTE_DEPENDENCIES, but not in lines 1 and 2 (i.e. keep calculating their
// C^+(X)). This is the solution in the Metanome implementation.

// The e(X) − e(X ∪ {A}) ≤ e(X → A) ≤ e(X) error bound optimization is omitted to make the algorithm
// easier to generalize to other measures: figuring out how to define them for keys would take some
// effort, and it is not guaranteed that the bounds would hold up under the most sensible
// generalization.

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

void Tane::Prune(CandidatesMap& rhs_candidates, PartitionsMap const& plis,
                 PartitionsMap const& parent_plis) {
    RelationalSchema const* schema = relation_->GetSchema();
    for (auto candidates_it = rhs_candidates.begin(); candidates_it != rhs_candidates.end();) {
        assert(candidates_it->second.any());
        if (!IsKey(plis.find(candidates_it->first)->second.get())) {
            ++candidates_it;
            continue;
        }
        CandidatesMap::node_type node = rhs_candidates.extract(candidates_it++);
        boost::dynamic_bitset<>& column_combination = node.key();
        boost::dynamic_bitset<>& current_rhs_candidates = node.mapped();
        // The check in the original article is incorrect because we are not
        // guaranteed to have calculated C^+ of every sibling of the key.
        // A ∈ ⋂B∈X C+(X ∪ {A} \ {B}) means there is no FD X \ {B} -> A with B ∈ X,
        // so replace with a direct FD check.
        if (column_combination.count() == 1) {
            model::Index const set_bit = column_combination.find_first();
            bool was_candidate = current_rhs_candidates.test_set(set_bit, false);
            util::ForEachIndex(current_rhs_candidates, [&](model::Index rhs_index) {
                config::ErrorType error =
                        CalculateZeroAryFdError(&relation_->GetColumnData(rhs_index));
                if (error <= max_fd_error_) return;
                RegisterAfd(AFD(schema->GetVertical(column_combination),
                                *schema->GetColumn(rhs_index),
                                0.0 /* key -> attr is always a plain FD */,
                                relation_->GetSharedPtrSchema()));
            });
            if (was_candidate) {
                current_rhs_candidates.set(set_bit);
            }
            continue;
        }
        util::ForEachIndex(current_rhs_candidates, [&](model::Index rhs_index) {
            // rhs_candidates - column_combination without allocations
            if (column_combination.test(rhs_index)) return;
            for (model::Index i = column_combination.find_first();
                 i != boost::dynamic_bitset<>::npos; i = column_combination.find_next(i)) {
                column_combination.reset(i);
                model::PLIWS const* lhs_pli = parent_plis.find(column_combination)->second.get();
                column_combination.set(i);
                model::PLIWS const* rhs_pli = relation_->GetColumnData(rhs_index).GetPLWSIndex();
                std::unique_ptr<model::PLIWS> joint_pli = lhs_pli->Intersect(rhs_pli);
                config::ErrorType error = CalculateFdError(lhs_pli, rhs_pli, joint_pli.get());

                if (error <= max_fd_error_) return;
            };
            RegisterAfd(AFD(schema->GetVertical(column_combination), *schema->GetColumn(rhs_index),
                            0.0 /* key -> attr is always a plain FD */,
                            relation_->GetSharedPtrSchema()));
        });
    }
}

void Tane::ComputeDependencies(PartitionsMap const& plis, PartitionsMap const& parent_plis,
                               CandidatesMap& candidates) {
    RelationalSchema const* schema = relation_->GetSchema();
    CandidatesMap current_candidates;

    for (auto it = candidates.begin(); it != candidates.end();) {
        auto& [column_combination, rhs_candidates] = *it;
        assert(rhs_candidates.any());
        if (column_combination.count() == 1) {
            model::Index const rhs_index = column_combination.find_first();
            if (!rhs_candidates.test(rhs_index)) {
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
                it = candidates.erase(it);
            } else {
                rhs_candidates.reset(rhs_index);
                if (rhs_candidates.none()) {
                    it = candidates.erase(it);
                } else {
                    ++it;
                }
            }
            continue;
        }
        boost::dynamic_bitset<> intersection = column_combination & rhs_candidates;
        boost::dynamic_bitset<> parent = column_combination;
        model::PLIWS const* joint_pli = plis.find(column_combination)->second.get();
        util::ForEachIndex(intersection, [&](model::Index rhs_index) {
            parent.reset(rhs_index);
            model::PLIWS const* lhs_pli = parent_plis.find(parent)->second.get();
            model::PLIWS const* rhs_pli = relation_->GetColumnData(rhs_index).GetPLWSIndex();
            config::ErrorType error = CalculateFdError(lhs_pli, rhs_pli, joint_pli);
            if (error > max_fd_error_) {
                parent.set(rhs_index);
                return;
            }
            RegisterAfd(AFD(schema->GetVertical(parent), *schema->GetColumn(rhs_index), error,
                            relation_->GetSharedPtrSchema()));
            parent.set(rhs_index);
            rhs_candidates.reset(rhs_index);
            if (error == 0.0) rhs_candidates &= column_combination;
        });
        if (rhs_candidates.none()) {
            it = candidates.erase(it);
        } else {
            ++it;
        }
    }
}

// Exactly PrefixBlocks but the order of bits is inverted and RHS candidates are stored.
auto Tane::SuffixBlocks(CandidatesMap const& rhs_candidates) -> SuffixMap {
    SuffixMap map;
    for (auto const& [column_combination, current_rhs_candidates] : rhs_candidates) {
        // TODO: test if this does anything.
        if (column_combination.test(0) && column_combination.test(1)) continue;
        boost::dynamic_bitset<> suffix = column_combination;
        model::Index const non_suffix_column = suffix.find_first();
        suffix.reset(non_suffix_column);
        map[std::move(suffix)].emplace_back(non_suffix_column, &current_rhs_candidates);
    }
    return map;
}

auto Tane::GenerateNextLevel(CandidatesMap const& current_candidates,
                             PartitionsMap const& current_plis)
        -> std::pair<CandidatesMap, PartitionsMap> {
    CandidatesMap next_rhs_candidates;
    PartitionsMap next_plis;
    SuffixMap s_map = SuffixBlocks(current_candidates);
    for (auto s_map_it = s_map.begin(); s_map_it != s_map.end();) {
        SuffixMap::node_type node = s_map.extract(s_map_it++);
        std::vector<std::pair<model::Index, boost::dynamic_bitset<> const*>>&
                prev_level_combinations = node.mapped();
        if (prev_level_combinations.size() < 2) continue;
        std::ranges::sort(prev_level_combinations,
                          [](std::pair<model::Index, boost::dynamic_bitset<> const*> const& c1,
                             std::pair<model::Index, boost::dynamic_bitset<> const*> const& c2) {
                              return c1.first < c2.first;
                          });
        boost::dynamic_bitset<>& new_combination = node.key();
        for (auto outer_it = prev_level_combinations.begin(),
                  end_it = std::prev(prev_level_combinations.end());
             outer_it != end_it; ++outer_it) {
            new_combination.set(outer_it->first);
            for (auto inner_it = std::next(outer_it); inner_it != prev_level_combinations.end();
                 ++inner_it) {
                if (!outer_it->second->intersects(*inner_it->second)) continue;
                boost::dynamic_bitset<> next_candidates = *inner_it->second & *outer_it->second;
                new_combination.set(inner_it->first);
                bool in_next_level = true;
                // No point in checking anything earlier, we already know they exist because we got
                // them from SuffixBlocks.
                for (model::Index i = new_combination.find_next(inner_it->first);
                     i != boost::dynamic_bitset<>::npos; i = new_combination.find_next(i)) {
                    new_combination.reset(i);
                    auto it = current_candidates.find(new_combination);
                    new_combination.set(i);
                    if (it == current_candidates.end() || (next_candidates &= it->second).none()) {
                        in_next_level = false;
                        break;
                    }
                }
                if (in_next_level) {
                    next_rhs_candidates.try_emplace(new_combination, next_candidates);
                    // "A partition with respect to a larger attribute set X is computed when X is
                    // added to its level on line 6 of GENERATE_NEXT_LEVEL"
                    model::Index const excluded_column = outer_it->first;
                    new_combination.reset(excluded_column);
                    model::PLIWS const& old_pli = *current_plis.find(new_combination)->second;
                    new_combination.set(excluded_column);
                    next_plis.try_emplace(
                            new_combination,
                            old_pli.Intersect(
                                    relation_->GetColumnData(excluded_column).GetPLWSIndex()));
                }
                new_combination.reset(inner_it->first);
            }
            new_combination.reset(outer_it->first);
        }
    }
    return {std::move(next_rhs_candidates), std::move(next_plis)};
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
    CandidatesMap prev_rhs_candidates{
            {boost::dynamic_bitset<>(relation_->GetNumColumns()),
             std::move(boost::dynamic_bitset<>(relation_->GetNumColumns()).set())}};
    CandidatesMap rhs_candidates;
    PartitionsMap prev_partitions;
    PartitionsMap current_partitions;
    for (model::Index column_index = 0; column_index != relation_->GetNumColumns();
         ++column_index) {
        boost::dynamic_bitset<> column_combination(relation_->GetNumColumns());
        column_combination.set(column_index);
        rhs_candidates.try_emplace(
                column_combination,
                std::move(boost::dynamic_bitset<>(relation_->GetNumColumns()).set()));
        current_partitions.try_emplace(
                std::move(column_combination),
                std::make_unique<model::PLIWS>(
                        *relation_->GetColumnData(column_index).GetPLWSIndex()));
    }
    std::size_t level_index = 0;
    while (!rhs_candidates.empty()) {
        ComputeDependencies(current_partitions, prev_partitions, rhs_candidates);
        if (level_index == max_lhs_) break;
        Prune(rhs_candidates, current_partitions, prev_partitions);

        prev_partitions = std::move(current_partitions);
        std::tie(rhs_candidates, current_partitions) =
                GenerateNextLevel(rhs_candidates, prev_partitions);
        ++level_index;
    }
}

}  // namespace algos
