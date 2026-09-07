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
    struct AttributeSetData {
        boost::dynamic_bitset<> rhs_candidates;
        std::unique_ptr<model::PLIWS> pli;

        bool IsSuperkey() const noexcept {
            return pli == nullptr;
        }

        void MarkSuperkey() noexcept {
            pli = nullptr;
        }
    };

    struct NoSuffixAttributeSetDataReference {
        model::Index non_suffix_column;
        AttributeSetData const* info;
    };

    // TODO: use an array of values for faster iteration.
    using SuffixMap = std::unordered_map<boost::dynamic_bitset<>,
                                         std::vector<NoSuffixAttributeSetDataReference>>;

    using LevelAttributeSetsData = std::unordered_map<boost::dynamic_bitset<>, AttributeSetData>;

    using FirstLevelComputeDependenciesResult =
            std::tuple<std::vector<model::Index>, std::vector<model::Index>,
                       boost::dynamic_bitset<>>;
    using FirstLevelPruneResults = std::tuple<std::vector<model::Index>, std::vector<model::Index>,
                                              std::vector<model::Index>, std::vector<model::Index>,
                                              boost::dynamic_bitset<>>;

    config::ErrorType max_fd_error_;
    model::AfdMeasure afd_measure_;

    void ResetStateFd() final {}

    boost::dynamic_bitset<> CreateEmptyColumnMask() {
        return boost::dynamic_bitset<>(relation_->GetNumColumns());
    }

    FirstLevelComputeDependenciesResult ComputeDependenciesLevel1();
    FirstLevelPruneResults PruneLevel1(std::vector<model::Index> const& inexact_zeroary_afd_rhss,
                                       std::vector<model::Index> const& not_zeroary_afd_rhss,
                                       boost::dynamic_bitset<> const& not_exact_zeroary_afd_rhss);
    void ComputeDependenciesLevel2KeysNotZeroaryAfdRhs(
            std::vector<model::Index> const& non_key_attrs_not_0afd,
            std::vector<model::Index> const& key_attrs_not_0afd,
            std::vector<model::Index> const& non_key_attrs_0afd,
            std::vector<model::Index> const& key_attrs_0afd,
            boost::dynamic_bitset<>& superkey_rhs_candidates,
            LevelAttributeSetsData& current_level);
    void ComputeDependenciesLevel2KeysZeroaryAfdRhs(
            std::vector<model::Index> const& non_key_attrs_not_0afd,
            std::vector<model::Index> const& non_key_attrs_0afd,
            std::vector<model::Index> const& key_attrs_0afd,
            boost::dynamic_bitset<>& superkey_rhs_candidates,
            LevelAttributeSetsData& current_level);
    void ComputeDependenciesLevel2NonKeysZeroaryAfdRhsPairs(
            boost::dynamic_bitset<> const& not_exact_zeroary_afd_rhss,
            std::vector<model::Index> const& non_key_attrs_0afd,
            LevelAttributeSetsData& current_level);
    void ComputeDependenciesLevel2NonKeysNotBothZeroaryAfdRhs(
            boost::dynamic_bitset<> const& not_exact_zeroary_afd_rhss,
            std::vector<model::Index> const& non_key_attrs_not_0afd,
            std::vector<model::Index> const& non_key_attrs_0afd,
            LevelAttributeSetsData& current_level);
    void ComputeDependenciesLevel2NonKeys(boost::dynamic_bitset<> const& not_exact_zeroary_afd_rhss,
                                          std::vector<model::Index> const& non_key_attrs_not_0afd,
                                          std::vector<model::Index> const& non_key_attrs_0afd,
                                          LevelAttributeSetsData& current_level);
    void ComputeDependenciesLevel2Keys(std::vector<model::Index> const& non_key_attrs_not_0afd,
                                       std::vector<model::Index> const& key_attrs_not_0afd,
                                       std::vector<model::Index> const& non_key_attrs_0afd,
                                       std::vector<model::Index> const& key_attrs_0afd,
                                       boost::dynamic_bitset<>& superkey_rhs_candidates,
                                       LevelAttributeSetsData& current_level);
    LevelAttributeSetsData ComputeDependenciesLevel2(
            boost::dynamic_bitset<> const& not_exact_zeroary_afd_rhss,
            std::vector<model::Index> const& non_key_attrs_not_0afd,
            std::vector<model::Index> const& key_attrs_not_0afd,
            std::vector<model::Index> const& non_key_attrs_0afd,
            std::vector<model::Index> const& key_attrs_0afd,
            boost::dynamic_bitset<>& superkey_rhs_candidates);
    void Prune(LevelAttributeSetsData& level);
    void ComputeDependencies(LevelAttributeSetsData& level,
                             LevelAttributeSetsData const& prev_level);
    // Exactly PrefixBlocks but the order of bits is inverted
    static SuffixMap SuffixBlocks(LevelAttributeSetsData const& level);
    LevelAttributeSetsData GenerateNextLevel(LevelAttributeSetsData const& level);
    void ExecuteInternal() final;
    void MakeExecuteOptsAvailableFDInternal() final;
    config::ErrorType CalculateZeroAryFdError(ColumnData const* rhs);
    config::ErrorType CalculateFdError(model::PLIWS const* lhs_pli, model::PLIWS const* rhs_pli,
                                       model::PLIWS const* joint_pli);

public:
    Tane();
};

}  // namespace algos
