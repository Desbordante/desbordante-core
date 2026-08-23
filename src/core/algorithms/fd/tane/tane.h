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
    struct ColumnCombinationMetadata {
        // We technically don't need this member from the previous level, but it's relatively small.
        boost::dynamic_bitset<> rhs_candidates;
        model::PLIWithSingletons position_list_index;
    };

    config::ErrorType max_fd_error_;
    model::AfdMeasure afd_measure_;

    using LatticeLevel = std::map<boost::dynamic_bitset<>, std::unique_ptr<model::LatticeVertex>>;
    using Level = std::unordered_map<boost::dynamic_bitset<>, ColumnCombinationMetadata>;

    void ResetStateFd() final {}

    LatticeLevel GenerateLevel1(model::LatticeVertex const* empty_vertex);
    void Prune(LatticeLevel& level);
    void ComputeDependencies(LatticeLevel& level);
    // Exactly PrefixBlocks but the order of bits is inverted
    // TODO: suffix array? Not going to do much for <64 columns, I think?
    std::unordered_map<boost::dynamic_bitset<>,
                       std::vector<std::pair<model::Index, model::LatticeVertex const*>>>
    SuffixBlocks(Level const& level);
    static LatticeLevel GenerateNextLevel(LatticeLevel& level);
    void ExecuteInternal() final;
    void MakeExecuteOptsAvailableFDInternal() final;
    config::ErrorType CalculateZeroAryFdError(ColumnData const* rhs);
    config::ErrorType CalculateFdError(model::PLIWS const* lhs_pli, model::PLIWS const* rhs_pli,
                                       model::PLIWS const* joint_pli);
    bool IsKey(model::PositionListIndex const* pli);
    static double CalculateUccError(model::PositionListIndex const* pli,
                                    ColumnLayoutRelationData const* relation_data);

public:
    Tane();
};

}  // namespace algos
