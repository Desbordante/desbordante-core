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
        // TODO: use a special pointer as PLI instead of this.
        bool is_part_of_level;
        std::unique_ptr<model::PLIWithSingletons> position_list_index;
    };

    struct PrevLevelColumnCombinationInfo {
        model::Index non_suffix_column;
        ColumnCombinationMetadata const* metadata;
    };

    // TODO: suffix array? Not going to do much for <64 columns, I think?
    using SuffixMap = std::unordered_map<boost::dynamic_bitset<>,
                                         std::vector<PrevLevelColumnCombinationInfo>>;

    config::ErrorType max_fd_error_;
    model::AfdMeasure afd_measure_;

    // TODO: rename, also stores C^+(X)
    using Level = std::unordered_map<boost::dynamic_bitset<>, ColumnCombinationMetadata>;

    void ResetStateFd() final {}

    void Prune(Level& level);
    void ComputeDependencies(Level& current_level, Level const& prev_level);
    // Exactly PrefixBlocks but the order of bits is inverted
    static SuffixMap SuffixBlocks(Level const& level);
    static Level GenerateNextLevel(Level& level);
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
