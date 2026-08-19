#include "core/algorithms/fd/tane/tane.h"

#include <iomanip>
#include <list>
#include <memory>

#include "core/algorithms/fd/afd_metric/afd_metric_calculator.h"
#include "core/algorithms/fd/tane/model/lattice_level.h"
#include "core/algorithms/fd/tane/model/lattice_vertex.h"
#include "core/config/error/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/model/table/column_data.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/util/logger.h"

namespace algos {
using boost::dynamic_bitset;
using Cluster = model::PositionListIndex::Cluster;

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

void Tane::Prune(model::LatticeLevel* level) {
    RelationalSchema const* schema = relation_->GetSchema();
    std::list<model::LatticeVertex*> key_vertices;
    for (auto& [map_key, vertex] : level->GetVertices()) {
        if (!vertex->GetIsKeyCandidate()) continue;

        /* probably incorrect */
        double ucc_error = CalculateUccError(vertex->GetPositionListIndex(), relation_.get());

        if (ucc_error > max_ucc_error_) continue;  // If a key candidate is not an approx UCC

        vertex->SetKeyCandidate(false);
        if (ucc_error != 0) continue;  // HUH?

        boost::dynamic_bitset<> columns = vertex->GetVertical();

        for (std::size_t rhs_index = vertex->GetRhsCandidates().find_first();
             rhs_index != boost::dynamic_bitset<>::npos;
             rhs_index = vertex->GetRhsCandidates().find_next(rhs_index)) {
            if (columns.test(rhs_index)) continue;  // for each A ∈ C+(X) \ X

            bool is_rhs_candidate = true;
            for (model::Index column = columns.find_first();
                 column != boost::dynamic_bitset<>::npos; column = columns.find_next(column)) {
                columns.reset(column);
                columns.set(rhs_index);
                auto sibling_vertex = level->GetLatticeVertex(columns);
                columns.reset(rhs_index);
                columns.set(column);

                if (sibling_vertex == nullptr ||
                    !sibling_vertex->GetConstRhsCandidates()[rhs_index]) {
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
                RegisterAfd(AFD(schema->GetVertical(columns), *schema->GetColumn(rhs_index),
                                fd_error, relation_->GetSharedPtrSchema()));
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

void Tane::ComputeDependencies(model::LatticeLevel* level) {
    RelationalSchema const* schema = relation_->GetSchema();
    for (auto& [key_map, xa_vertex] : level->GetVertices()) {
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
        for (auto const& x_vertex : xa_vertex->GetParents()) {
            dynamic_bitset<> const& lhs = x_vertex->GetVertical();

            // Find index of A in XA.
            dynamic_bitset<> differing_bits = xa_indices ^ lhs;
            std::size_t a_index = differing_bits.find_first();
            if (!a_candidates[a_index]) {
                continue;
            }
            auto x_pli = x_vertex->GetPositionListIndexWithSingletons();
            auto a_pli = relation_->GetColumnData(a_index).GetPLWSIndex();
            // Check X -> A
            config::ErrorType error = CalculateFdError(x_pli, a_pli, xa_pli);
            if (error <= max_fd_error_) {  // if X \ {A} → A is valid
                RegisterAfd(AFD(schema->GetVertical(lhs), *schema->GetColumn(a_index), error,
                                relation_->GetSharedPtrSchema()));  // output X \ {A} → A
                xa_vertex->GetRhsCandidates().reset(a_index);       // remove A from C+(X)
                if (error == 0) {
                    xa_vertex->GetRhsCandidates() &= lhs;  // remove all B in R \ X from C+(X)
                }
            }
        }
    }
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
            return 1 - /*<- incorrect*/ afd_metric_calculator::AFDMetricCalculator::CalculateG2(
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

    LOG_DEBUG("{} has {} columns, {} rows, and a maximum NIP of {:2}.", schema->GetName(),
              relation_->GetNumColumns(), relation_->GetNumRows(), relation_->GetMaximumNip());

    for (auto& column : schema->GetColumns()) {
        double avg_partners = relation_->GetColumnData(column->GetIndex())
                                      .GetPositionListIndex()
                                      ->GetNepAsLong() *
                              2.0 / relation_->GetNumRows();
        LOG_DEBUG("*{}: every tuple has {:2} partners on average.", column->ToString(),
                  avg_partners);
    }

    // Initialize level 0
    std::vector<std::unique_ptr<model::LatticeLevel>> levels;
    auto level0 = std::make_unique<model::LatticeLevel>(0);
    // TODO: через указатели кажется надо переделать
    level0->Add(std::make_unique<model::LatticeVertex>(dynamic_bitset<>(schema->GetNumColumns())));
    model::LatticeVertex const* empty_vertex = level0->GetVertices().begin()->second.get();
    levels.push_back(std::move(level0));

    // Initialize level1
    dynamic_bitset<> zeroary_fd_rhs(schema->GetNumColumns());
    auto level1 = std::make_unique<model::LatticeLevel>(1);
    for (model::Index column = 0; column != schema->GetNumColumns(); ++column) {
        // for each attribute set vertex
        ColumnData const& column_data = relation_->GetColumnData(column);
        auto vertex = std::make_unique<model::LatticeVertex>(
                dynamic_bitset<>(schema->GetNumColumns()).set(column));

        vertex->SetAllRhsCandidates();
        vertex->GetParents().push_back(empty_vertex);
        vertex->SetKeyCandidate(true);
        vertex->SetPLIWithSingletons(column_data.GetPLWSIndex());

        // check FDs: 0->A
        double fd_error = CalculateZeroAryFdError(&column_data);
        if (fd_error <= max_fd_error_) {  // TODO: max_error
            zeroary_fd_rhs.set(column);
            RegisterAfd(AFD(schema->CreateEmptyVertical(), *schema->GetColumn(column), fd_error,
                            relation_->GetSharedPtrSchema()));

            vertex->GetRhsCandidates().reset(column);
            if (fd_error == 0) {
                vertex->GetRhsCandidates().reset();
            }
        }

        level1->Add(std::move(vertex));
    }

    for (auto& [key_map, vertex] : level1->GetVertices()) {
        dynamic_bitset<> column = vertex->GetVertical();
        vertex->GetRhsCandidates() -= zeroary_fd_rhs;  // remove already discovered zeroary FDs

        // вот тут костыль, чтобы вытянуть индекс колонки из вершины, в которой только один индекс
        ColumnData const& column_data = relation_->GetColumnData(column.find_first());
        double ucc_error = CalculateUccError(column_data.GetPositionListIndex(), relation_.get());
        if (ucc_error <= max_ucc_error_) {
            vertex->SetKeyCandidate(false);
            if (ucc_error == 0 && max_lhs_ != 0) {
                for (unsigned long rhs_index = vertex->GetRhsCandidates().find_first();
                     rhs_index < vertex->GetRhsCandidates().size();
                     rhs_index = vertex->GetRhsCandidates().find_next(rhs_index)) {
                    if (rhs_index != column.find_first()) {
                        auto x_pli = vertex->GetPositionListIndexWithSingletons();
                        auto a_pli = relation_->GetColumnData(rhs_index).GetPLWSIndex();
                        config::ErrorType fd_error =
                                CalculateFdError(x_pli, a_pli, x_pli->Intersect(a_pli).get());
                        if (fd_error > max_fd_error_) continue;
                        RegisterAfd(
                                AFD(schema->GetVertical(dynamic_bitset<>(schema->GetNumColumns())
                                                                .set(column.find_first())),
                                    *schema->GetColumn(rhs_index), fd_error,
                                    relation_->GetSharedPtrSchema()));
                    }
                }
                vertex->GetRhsCandidates() &= column;
                // set vertex invalid if we seek for exact dependencies
                if (max_fd_error_ == 0 && max_ucc_error_ == 0) {
                    vertex->SetInvalid(true);
                }
            }
        }
    }
    levels.push_back(std::move(level1));

    unsigned int max_arity =
            max_lhs_ == std::numeric_limits<unsigned int>::max() ? max_lhs_ : max_lhs_ + 1;
    for (unsigned int arity = 2; arity <= max_arity; arity++) {
        model::LatticeLevel::ClearLevelsBelow(levels, arity - 1);
        model::LatticeLevel::GenerateNextLevel(levels);

        model::LatticeLevel* level = levels[arity].get();
        LOG_TRACE("Checking {} {}-ary lattice vertices.", level->GetVertices().size(), arity);
        if (level->GetVertices().empty()) {
            break;
        }

        ComputeDependencies(level);

        if (arity == max_arity) {
            break;
        }

        Prune(level);
        // TODO: printProfilingData
    }

    LOG_DEBUG("Total FD count: {}", afd_collection_.Size());
    LOG_DEBUG("HASH: {}", Fletcher16());
}

}  // namespace algos
