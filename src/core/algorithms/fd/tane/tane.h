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
        std::unique_ptr<model::PLIWS> pli;

        bool IsSuperkey() const noexcept {
            return pli == nullptr;
        }

        void MarkSuperkey() noexcept {
            pli = nullptr;
        }
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

    using LevelColumnCombinationsInfo =
            std::unordered_map<boost::dynamic_bitset<>, ColumnCombinationInfo>;

    void ResetStateFd() final {}

    void Prune(LevelColumnCombinationsInfo& level);
    void ComputeDependencies(LevelColumnCombinationsInfo& level,
                             LevelColumnCombinationsInfo const& prev_level);
    // Exactly PrefixBlocks but the order of bits is inverted
    static SuffixMap SuffixBlocks(LevelColumnCombinationsInfo const& level);
    LevelColumnCombinationsInfo GenerateNextLevel(LevelColumnCombinationsInfo const& level);
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
