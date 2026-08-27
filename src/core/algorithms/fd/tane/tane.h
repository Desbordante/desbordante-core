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
    using SuffixMap = std::unordered_map<boost::dynamic_bitset<>, std::vector<model::Index>>;

    config::ErrorType max_fd_error_;
    model::AfdMeasure afd_measure_;

    using Level = std::unordered_set<boost::dynamic_bitset<>>;
    using CandidatesMap = std::unordered_map<boost::dynamic_bitset<>, boost::dynamic_bitset<>>;
    using PartitionsMap =
            std::unordered_map<boost::dynamic_bitset<>, std::unique_ptr<model::PLIWS>>;

    void ResetStateFd() final {}

    void Prune(Level& level, CandidatesMap& candidates_map, PartitionsMap const& plis);
    void ComputeDependencies(Level const& level, PartitionsMap const& plis,
                             CandidatesMap& candidates_map);
    // Exactly PrefixBlocks but the order of bits is inverted
    static SuffixMap SuffixBlocks(Level const& level);
    Level GenerateNextLevel(Level level, PartitionsMap& plis);
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
