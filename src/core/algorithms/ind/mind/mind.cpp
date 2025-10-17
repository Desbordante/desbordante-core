/** \file
 * \brief Mind algorithm
 *
 * Mind algorithm class methods definition
 */
#include "mind.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <iterator>
#include <list>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/container_hash/hash.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/type_index.hpp>

#include "algorithm.h"
#include "algorithm_types.h"
#include "algorithms/create_algorithm.h"
#include "common_option.h"
#include "config/error/option.h"
#include "config/names_and_descriptions.h"
#include "error/type.h"
#include "exceptions.h"
#include "ind/ind.h"
#include "ind/ind_algorithm.h"
#include "ind/mind/raw_ind.h"
#include "max_arity/option.h"
#include "model/table/dataset_stream_projection.h"
#include "names_and_descriptions.h"
#include "table/arity_index.h"
#include "table/column_combination.h"
#include "table/dataset_stream_fixed.h"
#include "table/idataset_stream.h"
#include "table/tuple_index.h"
#include "tabular_data/input_table_type.h"
#include "util/timed_invoke.h"

namespace algos {

Mind::Mind() : INDAlgorithm({}) {
    MakeLoadOptsAvailable();

    RegisterOption(config::kErrorOpt(&max_ind_error_));
    RegisterOption(config::kMaxArityOpt(&max_arity_));
}

void Mind::MakeLoadOptsAvailable() {
    /*
     * At the moment we only have one algorithm for mining unary approximate inds.
     * In the future we should give the user the ability to choose the algorithm.
     */
    auind_algo_ = CreateAlgorithmInstance<INDAlgorithm>(AlgorithmType::spider);
}

void Mind::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kErrorOpt.GetName(), config::kMaxArityOpt.GetName()});
}

bool Mind::SetExternalOption(std::string_view option_name, boost::any const& value) {
    try {
        auind_algo_->SetOption(option_name, value);
        return true;
    } catch (config::ConfigurationError&) {
    }
    return false;
}

std::type_index Mind::GetExternalTypeIndex(std::string_view option_name) const {
    return auind_algo_->GetTypeIndex(option_name);
};

void Mind::LoadINDAlgorithmDataInternal() {
    timings_.load = util::TimedInvoke(&Algorithm::LoadData, auind_algo_);
}

void Mind::AddSpecificNeededOptions(std::unordered_set<std::string_view>& previous_options) const {
    auto const& approx_options = auind_algo_->GetNeededOptions();
    previous_options.insert(approx_options.begin(), approx_options.end());
}

namespace mind {
namespace {

/**
 * Create candidate using two valid inds from the previous level.
 *
 * @note Implements 2 line from `Algorithm 3 GenNext`.
 *       See "Unary and n-ary inclusion dependency discovery in relational databases".
 */
RawIND CreateCandidate(model::IND const& p, model::IND const& q) {
    auto const create_cc = [&p, &q](auto get_cc) {
        return std::invoke(get_cc, p).WithColumn(std::invoke(get_cc, q).GetLastColumn());
    };
    return {create_cc(&model::IND::GetLhs), create_cc(&model::IND::GetRhs)};
};

/**
 * Check if we should prune candidate.
 *
 * @note Implements check for 11-15 lines from `Algorithm 3 GenNext`.
 *       See "Unary and n-ary inclusion dependency discovery in relational databases".
 *
 * @note We do not put this into a separate stage after generating all the candidates,
 *       and perform this check immediately.
 */
bool CanPruneCandidate(RawIND const& candidate, std::unordered_set<RawIND> const& prev_inds) {
    model::ArityIndex const arity = candidate.lhs.GetArity();
    if (arity <= 2) return false;

    auto const candidate_without_id = [&candidate](model::ArityIndex id) -> RawIND {
        return {candidate.lhs.WithoutIndex(id), candidate.rhs.WithoutIndex(id)};
    };

    for (model::ArityIndex i = 0; i != arity; ++i) {
        if (!prev_inds.contains(candidate_without_id(i))) {
            return true;
        }
    }

    return false;
}

/**
 * Try to create new candidate using two valid inds from the previous level.
 *
 * @note This function implements main logic from `Algorithm 3 GenNext` listing (lines 4-16).
 *       See "Unary and n-ary inclusion dependency discovery in relational databases".
 *
 * @note We can optimize this function, but this will not improve the computing stage of the
 *       algorithm, since other stages takes many times longer.
 *
 * @return Candidate if valid, `std::nullopt` otherwise.
 */
std::optional<RawIND> GetCandidateIfValid(model::IND const& p, model::IND const& q,
                                          std::unordered_set<RawIND> const& prev_raw_inds) {
    if (!p.StartsWith(q)) {
        return std::nullopt;
    }

    if (!(p.GetLhs().GetLastColumn() < q.GetLhs().GetLastColumn() &&
          p.GetRhs().GetLastColumn() != q.GetRhs().GetLastColumn())) {
        return std::nullopt;
    }

    RawIND candidate{CreateCandidate(p, q)};
    if (model::ColumnCombination::HaveIndicesIntersection(candidate.lhs, candidate.rhs)) {
        return std::nullopt;
    }

    if (mind::CanPruneCandidate(candidate, prev_raw_inds)) {
        return std::nullopt;
    }

    return candidate;
}

/*
 * Firstly, skip rows with incorrect count of values.
 * Secondly, project stream on the provided column indices.
 */
using DatasetStreamFixedProjection = model::DatasetStreamProjection<model::DatasetStreamFixed<>>;

using Row = DatasetStreamFixedProjection::Row;
using HashSet = std::unordered_set<Row, boost::hash<Row>>;

HashSet CreateHashSet(DatasetStreamFixedProjection&& stream) {
    HashSet hash_container;

    while (stream.HasNextRow()) {
        hash_container.insert(stream.GetNextRow());
    }

    return hash_container;
}

}  // namespace
}  // namespace mind

std::optional<config::ErrorType> Mind::TestCandidate(RawIND const& raw_ind) {
    using namespace mind;

    auto const create_projected_stream = [&](model::ColumnCombination const& cc) {
        config::InputTable const& table = input_tables_[cc.GetTableIndex()];
        table->Reset();
        return DatasetStreamFixedProjection{table, cc.GetColumnIndices()};
    };

    HashSet const rhs_hash_set{CreateHashSet(create_projected_stream(raw_ind.rhs))};

    if (max_ind_error_ == 0) {
        DatasetStreamFixedProjection lhs_stream = create_projected_stream(raw_ind.lhs);
        while (lhs_stream.HasNextRow()) {
            if (!rhs_hash_set.contains(lhs_stream.GetNextRow())) {
                return std::nullopt;
            }
        }
        return config::ErrorType{0.0};
    }

    HashSet const lhs_hash_set{CreateHashSet(create_projected_stream(raw_ind.lhs))};
    auto const lhs_cardinality = static_cast<model::TupleIndex>(lhs_hash_set.size());
    model::TupleIndex const disqualify_row_limit = std::floor(lhs_cardinality * max_ind_error_) + 1;
    model::TupleIndex disqualify_row_count = 0;
    for (Row const& row : lhs_hash_set) {
        if (!rhs_hash_set.contains(row)) {
            ++disqualify_row_count;
            if (disqualify_row_count == disqualify_row_limit) {
                assert(static_cast<config::ErrorType>(disqualify_row_count) / lhs_cardinality >
                       max_ind_error_);
                return std::nullopt;
            }
        }
    }

    auto const error = static_cast<config::ErrorType>(disqualify_row_count) / lhs_cardinality;
    if (error <= max_ind_error_)
        return error;
    else
        return std::nullopt;
}

/*
 * Mine unary INDs.
 *
 * Redirect call to the algorithm for mining unary dependencies, then register dependencies.
 */
void Mind::MineUnaryINDs() {
    auind_algo_->Execute();
    for (const IND& ind : auind_algo_->INDList()) {
        RegisterIND(ind);
    }
}

/*
 * Mine n-ary INDs.
 */
void Mind::MineNaryINDs() {
    /* Current lattice level candidates. */
    std::vector<RawIND> candidates;
    /*
     * Iterator to the first element in the `INDList()` with previous level arity.
     * It's guaranteed (at the start of the loop), that range [`prev_it`, `INDList().end()`)
     * contains all elements with this arity, so we can avoid going through the entire `INDList()`
     * with arity check.
     */
    auto prev_it = INDList().begin();
    /*
     * Set of raw inds, that were found on the previous lattice level.
     * Note, that we do not populate this set for unary dependencies, since this
     * set is used to test the validity of candidates whose arity is at least 3.
     * (See `CanPruneCandidate()`)
     */
    std::unordered_set<RawIND> prev_raw_inds;

    /*
     * Stop INDs mining if no new dependencies were found at the previous lattice level
     * (from this condition it follows that no more dependencies can be found).
     */
    while (prev_it != INDList().end() && INDList().back().GetArity() != max_arity_) {
        for (auto p_it = prev_it; p_it != INDList().end(); ++p_it) {
            std::for_each(std::next(p_it), INDList().end(), [&](const IND& q) {
                std::optional<RawIND> candidate_opt =
                        mind::GetCandidateIfValid(*p_it, q, prev_raw_inds);
                if (candidate_opt) {
                    candidates.push_back(std::move(candidate_opt.value()));
                }
            });
        }

        prev_it = std::prev(INDList().end()); /*< last element of the previous lattice level */
        prev_raw_inds.clear();
        for (RawIND const& candidate : candidates) {
            if (auto error_opt = TestCandidate(candidate)) {
                RegisterIND(candidate.lhs, candidate.rhs, error_opt.value());
                prev_raw_inds.insert(candidate);
            }
        }
        candidates.clear();

        ++prev_it; /*< first element of the current lattice level or `INDList().end()` */
    };
}

unsigned long long Mind::ExecuteInternal() {
    timings_.compute_uinds = util::TimedInvoke(&Mind::MineUnaryINDs, this);
    timings_.compute_ninds = util::TimedInvoke(&Mind::MineNaryINDs, this);
    return timings_.compute_uinds + timings_.compute_ninds;
}

void Mind::ResetINDAlgorithmState() {
    timings_.compute_uinds = 0;
    timings_.compute_ninds = 0;
}

}  // namespace algos
