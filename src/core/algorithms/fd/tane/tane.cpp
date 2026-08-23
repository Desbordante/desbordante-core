#include "core/algorithms/fd/tane/tane.h"

#include <algorithm>
#include <list>
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

        // Is this COMPUTE_DEPENDENCIES(L_1)?
        // check FDs: 0->A
        double fd_error = CalculateZeroAryFdError(&column_data);
        if (fd_error <= max_fd_error_) {
            // if X \ {A} → A is valid
            // output X \ {A} → A
            RegisterAfd(AFD(schema->CreateEmptyVertical(), *schema->GetColumn(column), fd_error,
                            relation_->GetSharedPtrSchema()));
            zeroary_fd_rhs.set(column);
            // remove A from C^+(X)
            // remove all B in R \ X from C^+(X)
            // C^+(X) ends up being empty
            // prune proactively:
            // (2) if C+(X) = ∅ do (3) delete X from L`
            continue;
        }

        auto vertex = std::make_unique<LatticeVertex>(
                std::move(boost::dynamic_bitset<>(schema->GetNumColumns()).set(column)),
                // forall X ∈ L_1 C^+(X) = R
                std::move(boost::dynamic_bitset<>(schema->GetNumColumns()).set()), /*false,*/
                std::vector<LatticeVertex const*>{empty_vertex}, column_data.GetPLWSIndex());
        boost::dynamic_bitset<> const& vertical = vertex->GetVertical();
        current_level.emplace(vertical, std::move(vertex));
    }

    // PRUNE(L_1)?
    for (auto level_it = current_level.begin();
         level_it != current_level.end();) {  // for each X ∈ L_l do
        auto prev_it = level_it;
        auto& [map_key, vertex] = *level_it++;
        // for each X ∈ L_1 do
        dynamic_bitset<> const& column = vertex->GetVertical();
        vertex->GetRhsCandidates() -= zeroary_fd_rhs;  // remove already discovered zeroary FDs

        // TODO: figure out how to make use of this
        assert(column.count() == 1);
        model::Index const lhs_column_index = column.find_first();
        ColumnData const& column_data = relation_->GetColumnData(lhs_column_index);
        double ucc_error = CalculateUccError(column_data.GetPositionListIndex(), relation_.get());
        if (ucc_error != 0) continue;
        // The LHS column only has unique values
        // if X is a (super)key do
        if (max_lhs_ != 0) {
            vertex->GetRhsCandidates().reset(lhs_column_index);
            util::ForEachIndex(vertex->GetRhsCandidates(), [&](model::Index rhs_index) {
                // for each A ∈ C^+(X) \ X do
                assert(rhs_index != lhs_column_index);
                // ⋂B∈X C^+(X ∪ {A} \ {B}) with X = {lhs_column_index}
                // This is C^+({rhs_index}) = R, so the condition on line 6 of COMPUTE_DEPENDENCIES
                // holds
                RegisterAfd(AFD(
                        schema->GetVertical(std::move(
                                dynamic_bitset<>(schema->GetNumColumns()).set(lhs_column_index))),
                        *schema->GetColumn(rhs_index), 0.0, relation_->GetSharedPtrSchema()));
            });
        }
        level_it = current_level.erase(prev_it);
    }

    return current_level;
}

void Tane::Prune(LatticeLevel& level) {
    RelationalSchema const* schema = relation_->GetSchema();
    // std::list<LatticeVertex*> key_vertices;
    for (auto level_it = level.begin(); level_it != level.end();) {  // for each X ∈ L_l do
        auto prev_it = level_it;
        auto& [map_key, vertex] = *level_it++;
        if (vertex->GetRhsCandidates().none()) {
            level_it = level.erase(prev_it);
            continue;
        }
        double ucc_error = CalculateUccError(vertex->GetPositionListIndex(), relation_.get());
        if (ucc_error != 0) continue;  // if X is a (super)key

        boost::dynamic_bitset<> sibling_lhs_scratch = vertex->GetVertical();

        for (std::size_t rhs_index = vertex->GetRhsCandidates().find_first();
             rhs_index != boost::dynamic_bitset<>::npos;
             rhs_index = vertex->GetRhsCandidates().find_next(rhs_index)) {
            if (vertex->GetVertical().test(rhs_index)) continue;  // for each A ∈ C^+(X) \ X

            bool is_rhs_candidate = true;
            sibling_lhs_scratch.set(rhs_index);
            for (model::Index column = vertex->GetVertical().find_first();
                 column != boost::dynamic_bitset<>::npos;
                 column = vertex->GetVertical().find_next(column)) {
                sibling_lhs_scratch.reset(column);
                auto sibling_vertex_it = level.find(sibling_lhs_scratch);
                sibling_lhs_scratch.set(column);

                if (sibling_vertex_it == level.end() ||
                    !sibling_vertex_it->second->GetConstRhsCandidates()[rhs_index]) {
                    // if A ∈ ⋂B∈X C^+(X ∪ {A} \ {B})
                    is_rhs_candidate = false;
                    break;
                }
                // for each outer rhs: if there is a sibling s.t. it doesn't
                // have this rhs, there is no FD: vertex->rhs
            }
            sibling_lhs_scratch.reset(rhs_index);

            if (!is_rhs_candidate) continue;

            RegisterAfd(AFD(
                    schema->GetVertical(vertex->GetVertical()), *schema->GetColumn(rhs_index),
                    0.0 /* key -> attr is always a plain FD */, relation_->GetSharedPtrSchema()));
        }
        // delete X from L_l
        level_it = level.erase(prev_it);
    }
}

void Tane::ComputeDependencies(LatticeLevel& level) {
    RelationalSchema const* schema = relation_->GetSchema();
    for (auto& [key_map, xa_vertex] : level) {
        // if (xa_vertex->GetIsInvalid()) continue;

        // Calculate XA PLI
        if (xa_vertex->GetPositionListIndex() == nullptr) {
            auto parent_pli_1 = xa_vertex->GetParents()[0]->GetPositionListIndexWithSingletons();
            auto parent_pli_2 = xa_vertex->GetParents()[1]->GetPositionListIndexWithSingletons();
            xa_vertex->AcquirePLIWithSingletons(parent_pli_1->Intersect(parent_pli_2));
        }

        dynamic_bitset<> const& xa_indices = xa_vertex->GetVertical();
        dynamic_bitset<> const& a_candidates = xa_vertex->GetRhsCandidates();
        dynamic_bitset<> new_a_candidates = a_candidates;
        auto xa_pli = xa_vertex->GetPositionListIndexWithSingletons();
        for (LatticeVertex const* x_vertex : xa_vertex->GetParents()) {
            // parent_lhs is X \ {A}
            dynamic_bitset<> const& parent_lhs = x_vertex->GetVertical();

            // Find index of A in XA.
            dynamic_bitset<> differing_bits = xa_indices ^ /*- ?*/ parent_lhs;
            assert(differing_bits.count() == 1);
            std::size_t a_index = differing_bits.find_first();
            // if A is not an RHS candidate, then X \ {A} → A is not valid, no point in going
            // through with error calculation.
            if (!a_candidates[a_index]) continue;

            auto x_pli = x_vertex->GetPositionListIndexWithSingletons();
            auto a_pli = relation_->GetColumnData(a_index).GetPLWSIndex();
            // Check X -> A
            config::ErrorType error = CalculateFdError(x_pli, a_pli, xa_pli);
            if (error > max_fd_error_) continue;
            // if X \ {A} → A is valid
            // output X \ {A} → A
            RegisterAfd(AFD(schema->GetVertical(parent_lhs), *schema->GetColumn(a_index), error,
                            relation_->GetSharedPtrSchema()));
            // remove A from C+(X)
            xa_vertex->GetRhsCandidates().reset(a_index);

            if (error != 0) continue;
            // if X \ {A} → A holds exactly
            // remove all B in R \ X from C^+(X)
            new_a_candidates &= parent_lhs;
        }
        xa_vertex->GetRhsCandidates() &= new_a_candidates;
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

        if (vertex1.GetRhsCandidates().none()) {
            continue;
        }

        for (unsigned int vertex_index_2 = vertex_index_1 + 1;
             vertex_index_2 < current_level_vertices.size(); vertex_index_2++) {
            LatticeVertex& vertex2 = *current_level_vertices[vertex_index_2];

            // TODO: suffix, like in Depminer
            if (!vertex1.ComesBeforeAndSharePrefixWith(vertex2)) break;

            if (!vertex1.GetRhsCandidates().intersects(vertex2.GetRhsCandidates())) {
                continue;
            }

            boost::dynamic_bitset<> vertical = vertex1.GetVertical() | vertex2.GetVertical();
            boost::dynamic_bitset<> rhs_candidates =
                    vertex1.GetRhsCandidates() & vertex2.GetRhsCandidates();
            std::vector<LatticeVertex const*> parents;
            // parents.reserve(arity + 1);
            // bool is_invalid = vertex1.GetIsInvalid() || vertex2.GetIsInvalid();

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

                // if (parent_vertex.GetIsInvalid()) is_invalid = true;
            }

            {
                parents.push_back(&vertex1);
                parents.push_back(&vertex2);
                // for each X ∈ L_l do C+(X) := ⋂A∈X C+(X \ {A})
                auto child_vertex = std::make_unique<LatticeVertex>(
                        std::move(vertical), std::move(rhs_candidates), std::move(parents));
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
    for (auto col1_it = not_zeroary_afd_rhs.begin(), end_it = std::prev(not_zeroary_afd_rhs.end());
         col1_it != end_it; ++col1_it) {
        model::Index const col1 = *col1_it;
        for (auto col2_it = std::next(col1_it); col2_it != not_zeroary_afd_rhs.end(); ++col2_it) {
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
            // If an approximate FD holds, we don't test for its RHS on later levels because the
            // later AFDs would be non-minimal (line 7).
            // If an exact FD holds, we, in effect intersect the RHS candidates with the LHS.
            // In line 4 of COMPUTE_DEPENDENCIES we iterate over the intersection of C^+(X) and X.
            // Then, if X is a key, we iterate over C^+(X) \ X in PRUNE.

            // Lemma 3.2: Let B ∈ X and let X \ {B} → B be a valid dependency. If X → A holds, then
            // X \ {B} → A holds.
            // Thus, we know that an FD X \ {A} -> B will hold for sure, therefore X -> B is not
            // minimal. But if we are purely searching for AFDs, where our definition of "holds" is
            // e(X \ {A} -> A) <= eps, then the fact that e(X \ {A} -> A) <= eps does not on its own
            // imply that e(X -> B) <= eps. Or, at least, I don't see why it would and the article
            // doesn't say that either.

            // The article does not define the minimality of an AFD! But the definition for a FD as
            // given is: "A functional dependency X → A is minimal (in r) if A is not functionally
            // dependent on any proper subset of X, i.e. if Y → A does not hold in r for any Y ⊂ X."

            // AFD definition: "Given an error threshold ε, 0 ≤ ε ≤ 1, we say that X → A is an
            // approximate (functional) dependency if and only if e(X → A) is at most ε."

            // In effect, for this algorithm, an AFD X → A is minimal, if there is no AFD or FD with
            // the same RHS and a proper subset of X as LHS.
            // Hmm, I guess it's still correct, though?

            // X \ {A} -> B, A in another node as X \ {B} -> B?
            // If there is an FD X \ {A} -> B, then there is an AFD X \ {A} -> B. It's pruned here,
            // but where is it found?
            // With A != B we have four cases: A ∈ X, A ∉ X; B ∈ X, B ∉ X;
            // yy: trivial FD, not possible?
            // nn: X -> B, with X present in the previous level. During PRUNE? No, would have been
            // deleted. In a sibling as X' \ {B} -> B? Then it won't be in C^+(X) anyway.
            // yn: present in the sibling with or not deleted during previous PRUNE
            // ny: X -> B. Can potentially be found during PRUNE, approximate or not. If we skip
            // lines 8 and 9 and we don't find it during PRUNE, then the other level will have X ∪
            // {B}, and we'll find X' \ {B} -> {B} again.

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
                                          std::move(rhs_candidates), std::move(*joint_pli));
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
                rhs_candidates.set();
                rhs_candidates.reset(col2);
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
            }
        }
    }

    if (max_lhs_ == 1) return;

    // TODO: figure out what happens when LHS size reaches the maximum.
    for (unsigned int lhs_size = 2; lhs_size <= max_lhs_; ++lhs_size) {
        // PRUNE(L_{lhs_size})
        // GENERATE_NEXT_LEVEL(L_{lhs_size})

        // while L_l != ∅
        if (current_level.empty()) break;

        // COMPUTE_DEPENDENCIES(L_{lhs_size + 1})
    }
    /*
    RelationalSchema const* schema = relation_->GetSchema();
    // Initialize level 0
    LatticeLevel prev_level;
    LatticeVertex const* empty_vertex =
            prev_level
                    .emplace(dynamic_bitset<>(schema->GetNumColumns()),
                             std::make_unique<LatticeVertex>(
                                     dynamic_bitset<>(schema->GetNumColumns()),
                                     dynamic_bitset<>(schema->GetNumColumns()) //, false
                                     ))
                    .first->second.get();

    // Initialize level 1
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
    LOG_DEBUG("HASH: {}", Fletcher16());*/
}

}  // namespace algos
