#include "core/algorithms/fd/tane/tane_common.h"

#include <chrono>
#include <iomanip>
#include <list>
#include <memory>

#include "core/algorithms/fd/pli_based_fd_algorithm.h"
#include "core/algorithms/fd/tane/model/lattice_level.h"
#include "core/algorithms/fd/tane/model/lattice_vertex.h"
#include "core/config/error/option.h"
#include "core/model/table/column_data.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/relational_schema.h"
#include "core/util/logger.h"

namespace algos {
using boost::dynamic_bitset;

namespace tane {

TaneCommon::TaneCommon() : PliBasedFDAlgorithm({kDefaultPhaseName}) {
    RegisterOption(config::kErrorOpt(&max_ucc_error_));
}

double TaneCommon::CalculateUccError(model::PositionListIndex const* pli,
                                     ColumnLayoutRelationData const* relation_data) {
    return pli->GetNepAsLong() / static_cast<double>(relation_data->GetNumTuplePairs());
}

void TaneCommon::RegisterAndCountFd(Vertical lhs, Column const* rhs) {
    RegisterFd(std::move(lhs), *rhs, relation_->GetSharedPtrSchema());
}

void TaneCommon::Prune(model::LatticeLevel* level) {
    RelationalSchema const* schema = relation_->GetSchema();
    std::list<model::LatticeVertex*> key_vertices;
    for (auto& [map_key, vertex] : level->GetVertices()) {
        Vertical columns = vertex->GetVertical();  // Originally it's a ColumnCombination

        if (vertex->GetIsKeyCandidate()) {
            double ucc_error = CalculateUccError(vertex->GetPositionListIndex(), relation_.get());
            if (ucc_error <= max_ucc_error_) {  // If a key candidate is an approx UCC

                vertex->SetKeyCandidate(false);
                if (ucc_error == 0) {
                    for (std::size_t rhs_index = vertex->GetRhsCandidates().find_first();
                         rhs_index != boost::dynamic_bitset<>::npos;
                         rhs_index = vertex->GetRhsCandidates().find_next(rhs_index)) {
                        Vertical rhs = static_cast<Vertical>(*schema->GetColumn((int)rhs_index));
                        if (!columns.Contains(rhs)) {
                            bool is_rhs_candidate = true;
                            for (auto const& column : columns.GetColumns()) {
                                Vertical sibling =
                                        columns.Without(static_cast<Vertical>(*column)).Union(rhs);
                                auto sibling_vertex =
                                        level->GetLatticeVertex(sibling.GetColumnIndices());
                                if (sibling_vertex == nullptr ||
                                    !sibling_vertex->GetConstRhsCandidates()
                                             [rhs.GetColumnIndices().find_first()]) {
                                    is_rhs_candidate = false;
                                    break;
                                }
                                // for each outer rhs: if there is a sibling s.t. it doesn't
                                // have this rhs, there is no FD: vertex->rhs
                            }
                            // Found fd: vertex->rhs => register it
                            if (is_rhs_candidate) {
                                RegisterAndCountFd(columns, schema->GetColumn(rhs_index));
                            }
                        }
                    }
                    key_vertices.push_back(vertex.get());
                }
            }
        }
        // if we seek for exact FDs then SetInvalid
        if (max_fd_error_ == 0 && max_ucc_error_ == 0) {
            for (auto key_vertex : key_vertices) {
                key_vertex->GetRhsCandidates() &= key_vertex->GetVertical().GetColumnIndices();
                key_vertex->SetInvalid(true);
            }
        }
    }
}

void TaneCommon::ComputeDependencies(model::LatticeLevel* level) {
    RelationalSchema const* schema = relation_->GetSchema();
    for (auto& [key_map, xa_vertex] : level->GetVertices()) {
        if (xa_vertex->GetIsInvalid()) {
            continue;
        }
        Vertical xa = xa_vertex->GetVertical();
        // Calculate XA PLI
        if (xa_vertex->GetPositionListIndex() == nullptr) {
            auto parent_pli_1 = xa_vertex->GetParents()[0]->GetPositionListIndexWithSingletons();
            auto parent_pli_2 = xa_vertex->GetParents()[1]->GetPositionListIndexWithSingletons();
            xa_vertex->AcquirePLIWithSingletons(parent_pli_1->Intersect(parent_pli_2));
        }

        dynamic_bitset<> xa_indices = xa.GetColumnIndices();
        dynamic_bitset<> a_candidates = xa_vertex->GetRhsCandidates();
        auto xa_pli = xa_vertex->GetPositionListIndexWithSingletons();
        for (auto const& x_vertex : xa_vertex->GetParents()) {
            Vertical const& lhs = x_vertex->GetVertical();

            // Find index of A in XA.
            dynamic_bitset<> differing_bits = xa_indices ^ lhs.GetColumnIndices();
            std::size_t a_index = differing_bits.find_first();
            if (!a_candidates[a_index]) {
                continue;
            }
            auto x_pli = x_vertex->GetPositionListIndexWithSingletons();
            auto a_pli = relation_->GetColumnData(a_index).GetPLWSIndex();
            // Check X -> A
            config::ErrorType error = CalculateFdError(x_pli, a_pli, xa_pli);
            if (error <= max_fd_error_) {
                Column const* rhs = schema->GetColumns()[a_index].get();

                RegisterAndCountFd(lhs, rhs);
                xa_vertex->GetRhsCandidates().set(rhs->GetIndex(), false);
                if (error == 0) {
                    xa_vertex->GetRhsCandidates() &= lhs.GetColumnIndices();
                }
            }
        }
    }
}

unsigned long long TaneCommon::ExecuteInternal() {
    long apriori_millis = 0;
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
    auto start_time = std::chrono::system_clock::now();
    double progress_step = 100.0 / (schema->GetNumColumns() + 1);

    // Initialize level 0
    std::vector<std::unique_ptr<model::LatticeLevel>> levels;
    auto level0 = std::make_unique<model::LatticeLevel>(0);
    // TODO: через указатели кажется надо переделать
    level0->Add(std::make_unique<model::LatticeVertex>(schema->CreateEmptyVertical()));
    model::LatticeVertex const* empty_vertex = level0->GetVertices().begin()->second.get();
    levels.push_back(std::move(level0));
    AddProgress(progress_step);

    // Initialize level1
    dynamic_bitset<> zeroary_fd_rhs(schema->GetNumColumns());
    auto level1 = std::make_unique<model::LatticeLevel>(1);
    for (auto& column : schema->GetColumns()) {
        // for each attribute set vertex
        ColumnData const& column_data = relation_->GetColumnData(column->GetIndex());
        auto vertex = std::make_unique<model::LatticeVertex>(static_cast<Vertical>(*column));

        vertex->AddRhsCandidates(schema->GetColumns());
        vertex->GetParents().push_back(empty_vertex);
        vertex->SetKeyCandidate(true);
        vertex->SetPLIWithSingletons(column_data.GetPLWSIndex());

        // check FDs: 0->A
        double fd_error = CalculateZeroAryFdError(&column_data);
        if (fd_error <= max_fd_error_) {  // TODO: max_error
            zeroary_fd_rhs.set(column->GetIndex());
            RegisterAndCountFd(schema->CreateEmptyVertical(), column.get());

            vertex->GetRhsCandidates().set(column->GetIndex(), false);
            if (fd_error == 0) {
                vertex->GetRhsCandidates().reset();
            }
        }

        level1->Add(std::move(vertex));
    }

    for (auto& [key_map, vertex] : level1->GetVertices()) {
        Vertical column = vertex->GetVertical();
        vertex->GetRhsCandidates() &=
                ~zeroary_fd_rhs;  //~ returns flipped copy <- removed already discovered zeroary FDs

        // вот тут костыль, чтобы вытянуть индекс колонки из вершины, в которой только один индекс
        ColumnData const& column_data =
                relation_->GetColumnData(column.GetColumnIndices().find_first());
        double ucc_error = CalculateUccError(column_data.GetPositionListIndex(), relation_.get());
        if (ucc_error <= max_ucc_error_) {
            vertex->SetKeyCandidate(false);
            if (ucc_error == 0 && max_lhs_ != 0) {
                for (unsigned long rhs_index = vertex->GetRhsCandidates().find_first();
                     rhs_index < vertex->GetRhsCandidates().size();
                     rhs_index = vertex->GetRhsCandidates().find_next(rhs_index)) {
                    if (rhs_index != column.GetColumnIndices().find_first()) {
                        RegisterAndCountFd(column, schema->GetColumn(rhs_index));
                    }
                }
                vertex->GetRhsCandidates() &= column.GetColumnIndices();
                // set vertex invalid if we seek for exact dependencies
                if (max_fd_error_ == 0 && max_ucc_error_ == 0) {
                    vertex->SetInvalid(true);
                }
            }
        }
    }
    levels.push_back(std::move(level1));
    AddProgress(progress_step);

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
        AddProgress(progress_step);
    }

    SetProgress(100);
    std::chrono::milliseconds elapsed_milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                  start_time);
    apriori_millis += elapsed_milliseconds.count();

    LOG_DEBUG("Time: {} milliseconds", apriori_millis);
    LOG_DEBUG("Intersection time: {} ms", model::PositionListIndex::micros_ / 1000);
    LOG_DEBUG("Total intersections: {}", model::PositionListIndex::intersection_count_);
    LOG_DEBUG("Total FD count: {}", fd_collection_.Size());
    LOG_DEBUG("HASH: {}", Fletcher16());
    return apriori_millis;
}

}  // namespace tane

}  // namespace algos
