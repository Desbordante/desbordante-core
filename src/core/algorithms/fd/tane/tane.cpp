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

void Tane::Prune(Level& level, CandidatesMap const& candidates_map, PartitionsMap const& plis) {
    RelationalSchema const* schema = relation_->GetSchema();
    for (auto level_it = level.begin(); level_it != level.end();) {
        boost::dynamic_bitset<> const column_combination = *level_it++;
        if (candidates_map.find(column_combination)->second.none()) {
            level.erase(column_combination);
        }
        if (IsKey(plis.find(column_combination)->second.get())) {
            util::ForEachIndex(
                    candidates_map.find(column_combination)->second - column_combination,
                    [&](model::Index rhs_index) {
                        // The check in the original article is incorrect because we are not
                        // guaranteed to have calculated C^+ of every sibling of the key.
                        // A ∈ ⋂B∈X C+(X ∪ {A} \ {B}) means there is no FD X \ {B} -> A with B ∈ X,
                        // so replace with a direct FD check.
                        bool is_minimal = true;
                        util::ForEachIndex(column_combination, [&](model::Index i) {
                            config::ErrorType error;
                            if (column_combination.count() == 1) {
                                error = CalculateZeroAryFdError(
                                        &relation_->GetColumnData(rhs_index));
                            } else {
                                model::PLIWS const* lhs_pli =
                                        plis.find(boost::dynamic_bitset<>(column_combination)
                                                          .reset(i))
                                                ->second.get();
                                model::PLIWS const* rhs_pli =
                                        relation_->GetColumnData(rhs_index).GetPLWSIndex();
                                std::unique_ptr<model::PLIWS> joint_pli =
                                        lhs_pli->Intersect(rhs_pli);
                                error = CalculateFdError(lhs_pli, rhs_pli, joint_pli.get());
                            }
                            if (error > max_fd_error_) return;
                            is_minimal = false;
                        });
                        if (is_minimal) {
                            RegisterAfd(AFD(schema->GetVertical(column_combination),
                                            *schema->GetColumn(rhs_index),
                                            0.0 /* key -> attr is always a plain FD */,
                                            relation_->GetSharedPtrSchema()));
                        }
                    });
            level.erase(column_combination);
        }
    }
}

void Tane::ComputeDependencies(Level const& level, PartitionsMap const& plis,
                               CandidatesMap& candidates_map) {
    RelationalSchema const* schema = relation_->GetSchema();

    for (boost::dynamic_bitset<> const& column_combination : level) {
        boost::dynamic_bitset<> intersection(relation_->GetNumColumns());
        intersection.set();
        util::ForEachIndex(column_combination, [&](model::Index i) {
            intersection &=
                    candidates_map.find(boost::dynamic_bitset<>(column_combination).reset(i))
                            ->second;
        });
        candidates_map.try_emplace(column_combination, std::move(intersection));
    }
    for (boost::dynamic_bitset<> const& column_combination : level) {
        util::ForEachIndex(
                column_combination & candidates_map.find(column_combination)->second,
                [&](model::Index rhs_index) {
                    config::ErrorType error;
                    if (column_combination.count() == 1) {
                        error = CalculateZeroAryFdError(&relation_->GetColumnData(rhs_index));
                    } else {
                        model::PLIWS const* lhs_pli =
                                plis.find(boost::dynamic_bitset<>(column_combination)
                                                  .reset(rhs_index))
                                        ->second.get();
                        model::PLIWS const* rhs_pli =
                                relation_->GetColumnData(rhs_index).GetPLWSIndex();
                        model::PLIWS const* joint_pli = plis.find(column_combination)->second.get();
                        error = CalculateFdError(lhs_pli, rhs_pli, joint_pli);
                    }
                    if (error > max_fd_error_) return;
                    RegisterAfd(AFD(
                            schema->GetVertical(std::move(
                                    boost::dynamic_bitset<>(column_combination).reset(rhs_index))),
                            *schema->GetColumn(rhs_index), error, relation_->GetSharedPtrSchema()));
                    boost::dynamic_bitset<>& rhs_candidates =
                            candidates_map.find(column_combination)->second;
                    rhs_candidates.reset(rhs_index);
                    if (error == 0.0) rhs_candidates &= column_combination;
                });
    }
}

// Exactly PrefixBlocks but the order of bits is inverted.
auto Tane::SuffixBlocks(Level const& level) -> SuffixMap {
    SuffixMap map;
    for (auto const& column_combination : level) {
        boost::dynamic_bitset<> suffix = column_combination;
        model::Index const non_suffix_column = suffix.find_first();
        suffix.reset(non_suffix_column);
        map[std::move(suffix)].push_back(non_suffix_column);
    }
    return map;
}

auto Tane::GenerateNextLevel(Level level, PartitionsMap& plis) -> Level {
    Level next_level;
    SuffixMap s_map = SuffixBlocks(level);
    for (auto s_map_it = s_map.begin(); s_map_it != s_map.end();) {
        SuffixMap::node_type node = s_map.extract(s_map_it++);
        std::vector<model::Index>& indices = node.mapped();
        if (indices.size() < 2) continue;
        std::ranges::sort(indices);
        for (auto outer_it = indices.begin(), end_it = std::prev(indices.end()); outer_it != end_it;
             ++outer_it) {
            for (auto inner_it = std::next(outer_it); inner_it != indices.end(); ++inner_it) {
                boost::dynamic_bitset<> new_combination = node.key();
                new_combination.set(*inner_it).set(*outer_it);
                bool in_all_direct_subsets = true;
                util::ForEachIndex(new_combination, [&](model::Index i) {
                    if (!level.contains(boost::dynamic_bitset<>(new_combination).reset(i)))
                        in_all_direct_subsets = false;
                });
                if (in_all_direct_subsets) {
                    next_level.insert(new_combination);
                    // A partition with respect to a larger attribute set X is computed when X is
                    // added to its level on line 6 of GENERATE_NEXT_LEVEL
                    model::Index const excluded_column = new_combination.find_first();
                    plis.try_emplace(
                            std::move(new_combination),
                            plis.find(boost::dynamic_bitset<>(new_combination)
                                              .reset(excluded_column))
                                    ->second->Intersect(relation_->GetColumnData(excluded_column)
                                                                .GetPLWSIndex()));
                }
            }
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
    CandidatesMap rhs_candidates{
            {boost::dynamic_bitset<>(relation_->GetNumColumns()),
             std::move(boost::dynamic_bitset<>(relation_->GetNumColumns()).set())}};
    Level current_level;
    PartitionsMap partitions;
    for (model::Index column_index = 0; column_index != relation_->GetNumColumns();
         ++column_index) {
        boost::dynamic_bitset<> column_combination(relation_->GetNumColumns());
        column_combination.set(column_index);
        current_level.insert(column_combination);
        partitions.try_emplace(std::move(column_combination),
                               std::make_unique<model::PLIWS>(
                                       *relation_->GetColumnData(column_index).GetPLWSIndex()));
    }
    std::size_t level_number = 1;
    while (!current_level.empty()) {
        ComputeDependencies(current_level, partitions, rhs_candidates);
        if (level_number - 1 == max_lhs_) break;
        Prune(current_level, rhs_candidates, partitions);
        current_level = GenerateNextLevel(std::move(current_level), partitions);
        ++level_number;
    }
}

}  // namespace algos
