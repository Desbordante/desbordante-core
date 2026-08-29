#pragma once

#include <map>

#include "core/algorithms/fd/afd_measure.h"
#include "core/algorithms/fd/pli_based_afd_algorithm.h"
#include "core/config/error/type.h"
#include "core/model/table/column_data.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/position_list_index.h"

namespace algos {

class Tane final : public PliBasedAFDAlgorithm {
    // TODO: suffix array? Not going to do much for <64 columns, I think?
    using SuffixMap = std::unordered_map<
            boost::dynamic_bitset<>,
            std::vector<std::pair<model::Index, boost::dynamic_bitset<> const*>>>;
    // RHS candidates and level together, PLIs? If not key, will need the PLI, unless last level. If
    // checking level, will need RHS candidates.

    config::ErrorType max_fd_error_;
    model::AfdMeasure afd_measure_;

    // Merge Level and CandidatesMap? We have to do that for the no-prune mitigation, but what will
    // this do for the direct calculation mitigation? ComputeDependencies and Prune use
    // CandidatesMap, but we can also calculate RHS candidates in GenerateNextLevel. We need PLIs
    // from the previous level too. The previous implementation intersected lazily, only in
    // ComputeDependencies. This will allow us to register slightly more dependencies in the case we
    // run out of memory, but it doesn't really matter. GenerateNextLevel is executed after all the
    // dependency checking is done, so we don't need previous level's PLIs by that point.
    using CandidatesMap = std::unordered_map<boost::dynamic_bitset<>, boost::dynamic_bitset<>>;
    using PartitionsMap =
            std::unordered_map<boost::dynamic_bitset<>, std::unique_ptr<model::PLIWS>>;

    void ResetStateFd() final {}

    void Prune(CandidatesMap& rhs_candidates, PartitionsMap const& plis,
               PartitionsMap const& parent_plis);
    void ComputeDependencies(PartitionsMap const& plis, PartitionsMap const& parent_plis,
                             CandidatesMap& prev_candidates);
    // Exactly PrefixBlocks but the order of bits is inverted
    static SuffixMap SuffixBlocks(CandidatesMap const& rhs_candidates);
    std::pair<CandidatesMap, PartitionsMap> GenerateNextLevel(
            CandidatesMap const& current_candidates, PartitionsMap const& current_plis);
    void ExecuteInternal() final;
    void MakeExecuteOptsAvailableFDInternal() final;
    config::ErrorType CalculateZeroAryFdError(ColumnData const* rhs);
    config::ErrorType CalculateFdError(model::PLIWS const* lhs_pli, model::PLIWS const* rhs_pli,
                                       model::PLIWS const* joint_pli);
    bool IsKey(model::PositionListIndex const* pli);

public:
    Tane();
};

}  // namespace algos
