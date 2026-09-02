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
    struct ColumnCombinationInfo {
        boost::dynamic_bitset<> rhs_candidates;
        bool is_superkey;
    };

    struct NoSuffixColumnCombinationInfoRef {
        model::Index non_suffix_column;
        ColumnCombinationInfo const* info;
    };

    // TODO: suffix array? Not going to do much for <64 columns, I think?
    using SuffixMap = std::unordered_map<boost::dynamic_bitset<>,
                                         std::vector<NoSuffixColumnCombinationInfoRef>>;
    // RHS candidates and level together, what about PLIs? If not key, will need the PLI. On last
    // level (max_lhs_), no PLIs need to be saved. If checking level, will need RHS candidates.

    config::ErrorType max_fd_error_;
    model::AfdMeasure afd_measure_;

    using CandidatesMap = std::unordered_map<boost::dynamic_bitset<>, ColumnCombinationInfo>;
    using PartitionsMap =
            std::unordered_map<boost::dynamic_bitset<>, std::unique_ptr<model::PLIWS>>;

    void ResetStateFd() final {}

    void Prune(CandidatesMap& rhs_candidates, PartitionsMap const& plis);
    void ComputeDependencies(PartitionsMap const& plis, PartitionsMap const& parent_plis,
                             CandidatesMap& candidates);
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
