#include "core/algorithms/fd/tane/tane.h"

#include <algorithm>
#include <list>
#include <memory>
#include <ranges>

#include "core/algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "core/algorithms/fd/tane/model/lattice_vertex.h"
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
using model::LatticeVertex;

Tane::Tane() : PliBasedAFDAlgorithm() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::kErrorOpt(&max_ucc_error_));
    RegisterOption(Option{&afd_measure_, kAfdMeasure, kDAfdMeasure, model::AfdMeasure::kG1});
}

void Tane::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::kErrorOpt.GetName(), config::names::kAfdMeasure});
}

double Tane::CalculateUccError(model::PositionListIndex const* pli,
                               ColumnLayoutRelationData const* relation_data) {
    return pli->GetNepAsLong() / static_cast<double>(relation_data->GetNumTuplePairs());
}

auto Tane::GenerateLevel1(LatticeVertex const* empty_vertex) -> LatticeLevel {
    RelationalSchema const* schema = relation_->GetSchema();
    dynamic_bitset<> zeroary_fd_rhs(schema->GetNumColumns());
    LatticeLevel current_level;
    for (model::Index column = 0; column != schema->GetNumColumns(); ++column) {
        // for each attribute set vertex
        ColumnData const& column_data = relation_->GetColumnData(column);

        boost::dynamic_bitset<> rhs_candidates;

        // check FDs: 0->A
        double fd_error = CalculateZeroAryFdError(&column_data);
        if (fd_error <= max_fd_error_) {  // TODO: max_error
            zeroary_fd_rhs.set(column);
            RegisterAfd(AFD(schema->CreateEmptyVertical(), *schema->GetColumn(column), fd_error,
                            relation_->GetSharedPtrSchema()));
            // continue; ??? passed the tests

            if (fd_error == 0) {
                // there is an actual FD [] -> A
                continue;
            }
        }
        rhs_candidates.resize(schema->GetNumColumns(), true);

        auto vertex = std::make_unique<LatticeVertex>(
                std::move(boost::dynamic_bitset<>(schema->GetNumColumns()).set(column)),
                std::move(rhs_candidates), true, false,
                std::vector<LatticeVertex const*>{empty_vertex}, column_data.GetPLWSIndex());
        boost::dynamic_bitset<> const& vertical = vertex->GetVertical();
        current_level.emplace(vertical, std::move(vertex));
    }

    for (auto& [key_map, vertex] : current_level) {
        dynamic_bitset<> const& column = vertex->GetVertical();
        vertex->GetRhsCandidates() -= zeroary_fd_rhs;  // remove already discovered zeroary FDs

        // TODO: figure out how to make use of this
        assert(column.count() == 1);
        model::Index const lhs_column_index = column.find_first();
        ColumnData const& column_data = relation_->GetColumnData(lhs_column_index);
        double ucc_error = CalculateUccError(column_data.GetPositionListIndex(), relation_.get());
        if (ucc_error <= max_ucc_error_) {
            vertex->SetKeyCandidate(false);
            if (ucc_error == 0 && max_lhs_ != 0) {
                // The LHS column only has unique values
                bool const has_lhs_set =
                        vertex->GetRhsCandidates().test_set(lhs_column_index, false);
                util::ForEachIndex(vertex->GetRhsCandidates(), [&](model::Index rhs_index) {
                    assert(rhs_index != lhs_column_index);
                    auto x_pli = vertex->GetPositionListIndexWithSingletons();
                    auto a_pli = relation_->GetColumnData(rhs_index).GetPLWSIndex();
                    config::ErrorType fd_error =
                            CalculateFdError(x_pli, a_pli, x_pli->Intersect(a_pli).get());
                    if (fd_error > max_fd_error_) return;
                    RegisterAfd(AFD(
                            schema->GetVertical(std::move(dynamic_bitset<>(schema->GetNumColumns())
                                                                  .set(lhs_column_index))),
                            *schema->GetColumn(rhs_index), fd_error,
                            relation_->GetSharedPtrSchema()));
                });
                vertex->GetRhsCandidates().reset();
                if (has_lhs_set) vertex->GetRhsCandidates().set(lhs_column_index);
                // set vertex invalid if we seek for exact dependencies
                if (max_fd_error_ == 0 && max_ucc_error_ == 0) {
                    // Is it correct? Not all measures are g1, which is what is used for ucc_error.
                    vertex->SetInvalid(true);
                }
            }
        }
    }

    return current_level;
}

void Tane::Prune(LatticeLevel& level) {
    RelationalSchema const* schema = relation_->GetSchema();
    std::list<LatticeVertex*> key_vertices;
    for (auto& [map_key, vertex] : level) {  // for each X ∈ L_l do
        if (!vertex->GetIsKeyCandidate()) continue;

        /* probably incorrect */
        double ucc_error = CalculateUccError(vertex->GetPositionListIndex(), relation_.get());

        if (ucc_error > max_ucc_error_) continue;  // If a key candidate is not an approx UCC

        vertex->SetKeyCandidate(false);
        if (ucc_error != 0) continue;  // if X is a (super)key

        boost::dynamic_bitset<> lhs = vertex->GetVertical();

        for (std::size_t rhs_index = vertex->GetRhsCandidates().find_first();
             rhs_index != boost::dynamic_bitset<>::npos;
             rhs_index = vertex->GetRhsCandidates().find_next(rhs_index)) {
            if (lhs.test(rhs_index)) continue;  // for each A ∈ C^+(X) \ X

            bool is_rhs_candidate = true;
            for (model::Index column = lhs.find_first(); column != boost::dynamic_bitset<>::npos;
                 column = lhs.find_next(column)) {
                lhs.reset(column);
                lhs.set(rhs_index);
                auto sibling_vertex_it = level.find(lhs);
                lhs.reset(rhs_index);
                lhs.set(column);

                if (sibling_vertex_it == level.end() ||
                    !sibling_vertex_it->second->GetConstRhsCandidates()[rhs_index]) {
                    // if A ∈ ⋂B∈X C+(X ∪ {A} \ {B})
                    is_rhs_candidate = false;
                    break;
                }
                // for each outer rhs: if there is a sibling s.t. it doesn't
                // have this rhs, there is no FD: vertex->rhs
            }
            // Found fd: vertex->rhs => register it
            if (is_rhs_candidate) {
                auto x_pli = vertex->GetPositionListIndexWithSingletons();
                auto a_pli = relation_->GetColumnData(rhs_index).GetPLWSIndex();
                config::ErrorType fd_error =
                        CalculateFdError(x_pli, a_pli, x_pli->Intersect(a_pli).get());
                if (fd_error > max_fd_error_) continue;
                RegisterAfd(AFD(schema->GetVertical(lhs), *schema->GetColumn(rhs_index), fd_error,
                                relation_->GetSharedPtrSchema()));
            }
        }
        key_vertices.push_back(vertex.get());
    }
    // if we seek for exact FDs then SetInvalid
    if (max_fd_error_ == 0 && max_ucc_error_ == 0) {
        for (auto key_vertex : key_vertices) {
            key_vertex->GetRhsCandidates() &= key_vertex->GetVertical();
            key_vertex->SetInvalid(true);
        }
    }
}

void Tane::ComputeDependencies(LatticeLevel& level) {
    RelationalSchema const* schema = relation_->GetSchema();
    for (auto& [key_map, xa_vertex] : level) {
        if (xa_vertex->GetIsInvalid()) {
            continue;
        }
        // Calculate XA PLI
        if (xa_vertex->GetPositionListIndex() == nullptr) {
            auto parent_pli_1 = xa_vertex->GetParents()[0]->GetPositionListIndexWithSingletons();
            auto parent_pli_2 = xa_vertex->GetParents()[1]->GetPositionListIndexWithSingletons();
            xa_vertex->AcquirePLIWithSingletons(parent_pli_1->Intersect(parent_pli_2));
        }

        dynamic_bitset<> const& xa_indices = xa_vertex->GetVertical();
        dynamic_bitset<> const& a_candidates = xa_vertex->GetRhsCandidates();
        auto xa_pli = xa_vertex->GetPositionListIndexWithSingletons();
        for (LatticeVertex const* x_vertex : xa_vertex->GetParents()) {
            dynamic_bitset<> const& parent_lhs = x_vertex->GetVertical();

            // Find index of A in XA.
            dynamic_bitset<> differing_bits = xa_indices ^ /*- ?*/ parent_lhs;
            std::size_t a_index = differing_bits.find_first();
            if (!a_candidates[a_index]) {  // differing_bits.count() == 1? Makes sense
                continue;
            }
            auto x_pli = x_vertex->GetPositionListIndexWithSingletons();
            auto a_pli = relation_->GetColumnData(a_index).GetPLWSIndex();
            // Check X -> A
            config::ErrorType error = CalculateFdError(x_pli, a_pli, xa_pli);
            // if X \ {A} → A is valid
            if (error <= max_fd_error_) {
                // output X \ {A} → A
                RegisterAfd(AFD(schema->GetVertical(parent_lhs), *schema->GetColumn(a_index), error,
                                relation_->GetSharedPtrSchema()));
                // remove A from C+(X)
                xa_vertex->GetRhsCandidates().reset(a_index);
                // if X \ {A} → A holds exactly
                if (error == 0) {
                    // remove all B in R \ X from C+(X)
                    xa_vertex->GetRhsCandidates() &= parent_lhs;
                }
            }
        }
    }
}

auto Tane::GenerateNextLevel(LatticeLevel& current_level) -> LatticeLevel {
    assert(!current_level.empty());
    unsigned int const arity = current_level.begin()->second->GetVertical().count();
    std::vector<LatticeVertex*> current_level_vertices;
    for (auto const& [map_key, vertex] : current_level) {
        current_level_vertices.push_back(vertex.get());
    }

    std::sort(current_level_vertices.begin(), current_level_vertices.end(),
              LatticeVertex::Comparator);
    LatticeLevel next_level;

    for (unsigned int vertex_index_1 = 0; vertex_index_1 < current_level_vertices.size();
         vertex_index_1++) {
        LatticeVertex& vertex1 = *current_level_vertices[vertex_index_1];

        /* Get rid of GetIsKeyCandidate? */
        if (vertex1.GetRhsCandidates().none() && !vertex1.GetIsKeyCandidate()) {
            continue;
        }

        for (unsigned int vertex_index_2 = vertex_index_1 + 1;
             vertex_index_2 < current_level_vertices.size(); vertex_index_2++) {
            LatticeVertex& vertex2 = *current_level_vertices[vertex_index_2];

            if (!vertex1.ComesBeforeAndSharePrefixWith(vertex2)) {
                break;
            }

            if (!vertex1.GetRhsCandidates().intersects(vertex2.GetRhsCandidates()) &&
                !vertex2.GetIsKeyCandidate()) {
                continue;
            }

            boost::dynamic_bitset<> vertical = vertex1.GetVertical() | vertex2.GetVertical();
            boost::dynamic_bitset<> rhs_candidates =
                    vertex1.GetRhsCandidates() & vertex2.GetRhsCandidates();
            std::vector<LatticeVertex const*> parents;
            // parents.reserve(arity + 1);
            bool is_key_candidate = vertex1.GetIsKeyCandidate() && vertex2.GetIsKeyCandidate();
            bool is_invalid = vertex1.GetIsInvalid() || vertex2.GetIsInvalid();

            for (unsigned int i = 0, skip_index = vertical.find_first(); i < arity - 1;
                 i++, skip_index = vertical.find_next(skip_index)) {
                vertical.reset(skip_index);
                auto parent_vertex_it = current_level.find(vertical);

                if (parent_vertex_it == current_level.end()) {
                    goto notInNextLevel;
                }
                LatticeVertex const& parent_vertex = *parent_vertex_it->second;
                rhs_candidates &= parent_vertex.GetConstRhsCandidates();
                // For any node, its rhs_candidates (C^+(X)) is the intersection of all its parents'
                // rhs_candidates. So if any node has empty rhs_candidates, it will block other
                // nodes from having any RHSs in the answer. The same effect would be achieved if
                // they were never added in the first place, since GenerateNextLevel checks for the
                // presence of every node. Leap of faith: let's just delete them immediately and
                // assume they never exist!
                if (rhs_candidates.none()) {
                    goto notInNextLevel;
                }
                parents.push_back(&parent_vertex);
                vertical.set(skip_index);

                if (!parent_vertex.GetIsKeyCandidate()) is_key_candidate = false;
                if (parent_vertex.GetIsInvalid()) is_invalid = true;
            }

            {
                parents.push_back(&vertex1);
                parents.push_back(&vertex2);
                auto child_vertex = std::make_unique<LatticeVertex>(
                        std::move(vertical), std::move(rhs_candidates), is_key_candidate,
                        is_invalid, std::move(parents));
                boost::dynamic_bitset<> const& child_vertical = child_vertex->GetVertical();
                next_level.try_emplace(child_vertical, std::move(child_vertex));
            }

        notInNextLevel:
            continue;
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
            return 1 - /*<- incorrect?*/ afd_metric_calculator::AFDMetricCalculator::CalculateG2(
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
    max_fd_error_ = max_ucc_error_;
    RelationalSchema const* schema = relation_->GetSchema();

    // Initialize level 0
    LatticeLevel prev_level;
    LatticeVertex const* empty_vertex =
            prev_level
                    .emplace(dynamic_bitset<>(schema->GetNumColumns()),
                             std::make_unique<LatticeVertex>(
                                     dynamic_bitset<>(schema->GetNumColumns()),
                                     dynamic_bitset<>(schema->GetNumColumns()), false, false))
                    .first->second.get();

    // Initialize level 1
    dynamic_bitset<> zeroary_fd_rhs(schema->GetNumColumns());
    LatticeLevel current_level = GenerateLevel1(empty_vertex);

    unsigned int max_arity =
            max_lhs_ == std::numeric_limits<unsigned int>::max() ? max_lhs_ : max_lhs_ + 1;
    for (unsigned int arity = 2; arity <= max_arity; arity++) {
        std::ranges::for_each(
                current_level | std::views::values,
                [](std::unique_ptr<LatticeVertex>& vertex) { vertex->GetParents().clear(); });
        prev_level = std::move(current_level);
        current_level = GenerateNextLevel(prev_level);

        LOG_TRACE("Checking {} {}-ary lattice vertices.", current_level.size(), arity);
        if (current_level.empty()) {
            break;
        }

        ComputeDependencies(current_level);

        if (arity == max_arity) {
            break;
        }

        Prune(current_level);
    }

    LOG_DEBUG("Total FD count: {}", afd_collection_.Size());
    LOG_DEBUG("HASH: {}", Fletcher16());
}

}  // namespace algos
