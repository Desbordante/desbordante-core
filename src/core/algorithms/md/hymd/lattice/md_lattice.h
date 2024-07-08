#pragma once

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "algorithms/md/hymd/lattice/md.h"
#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/lattice/md_node.h"
#include "algorithms/md/hymd/lattice/node_base.h"
#include "algorithms/md/hymd/lattice/rhs.h"
#include "algorithms/md/hymd/lattice/single_level_func.h"
#include "algorithms/md/hymd/lattice/support_node.h"
#include "algorithms/md/hymd/md_element.h"
#include "algorithms/md/hymd/md_lhs.h"
#include "algorithms/md/hymd/pair_comparison_result.h"
#include "algorithms/md/hymd/rhss.h"
#include "algorithms/md/hymd/utility/invalidated_rhss.h"
#include "model/index.h"
#include "util/excl_optional.h"

namespace algos::hymd::lattice {

class MdLattice {
private:
    class GeneralizationHelper;

    using MdCCVIdChildMap = MdNode::OrderedCCVIdChildMap;
    using MdOptionalMap = MdNode::OptionalChildMap;
    using MdNodeChildren = MdNode::Children;

    /*static bool IsNotMax(ColumnClassifierValueId const& ccv_id) noexcept {
        return ccv_id != -1u;
    }

    static ColumnClassifierValueId CCVIdDefault() noexcept {
        return -1;
    }*/

public:
    // using OptionalCCvId = util::ExclOptional<ColumnClassifierValueId, IsNotMax, CCVIdDefault>;

    class MdRefiner {
        MdLattice* lattice_;
        PairComparisonResult const* pair_similarities_;
        MdLatticeNodeInfo node_info_;
        utility::InvalidatedRhss invalidated_;

    public:
        MdRefiner(MdLattice* lattice, PairComparisonResult const* pair_similarities,
                  MdLatticeNodeInfo node_info, utility::InvalidatedRhss invalidated)
            : lattice_(lattice),
              pair_similarities_(pair_similarities),
              node_info_(std::move(node_info)),
              invalidated_(std::move(invalidated)) {}

        MdLhs const& GetLhs() const {
            return node_info_.lhs;
        }

        void ZeroRhs() {
            node_info_.ZeroRhs();
        }

        std::size_t Refine();

        bool InvalidatedNumber() const noexcept {
            return invalidated_.Size();
        }
    };

    class MdVerificationMessenger {
        MdLattice* lattice_;
        MdLatticeNodeInfo node_info_;

    public:
        MdVerificationMessenger(MdLattice* lattice, MdLatticeNodeInfo node_info)
            : lattice_(lattice), node_info_(std::move(node_info)) {}

        MdLhs const& GetLhs() const {
            return node_info_.lhs;
        }

        Rhs& GetRhs() {
            return *node_info_.rhs;
        }

        void MarkUnsupported();

        void ZeroRhs() {
            node_info_.ZeroRhs();
        }

        void LowerAndSpecialize(utility::InvalidatedRhss const& invalidated);
    };

private:
    std::size_t max_level_ = 0;
    std::size_t const column_matches_size_;
    std::vector<std::vector<ColumnClassifierValueId>> const rhs_to_lhs_ccv_id_map_;
    MdNode md_root_;
    SupportNode support_root_;
    // Is there a way to define a level in such a way that one cannot use each CCV ID independently
    // to determine an MD's level but the lattice traversal algorithm still works?
    SingleLevelFunc const get_single_level_;
    std::vector<std::vector<ColumnClassifierValueId>> const* const lhs_ccv_ids_;
    bool const prune_nondisjoint_;
    std::size_t const max_cardinality_;

    [[nodiscard]] bool HasGeneralization(Md const& md) const;

    void GetLevel(MdNode& cur_node, std::vector<MdVerificationMessenger>& collected,
                  MdLhs& cur_node_lhs, model::Index cur_node_index, std::size_t level_left);

    void RaiseInterestingnessCCVIds(
            MdNode const& cur_node, MdLhs const& lhs,
            std::vector<ColumnClassifierValueId>& cur_interestingness_ccv_ids,
            MdLhs::iterator cur_lhs_iter, std::vector<model::Index> const& indices) const;

    void TryAddRefiner(std::vector<MdRefiner>& found, Rhs& rhs,
                       PairComparisonResult const& pair_comparison_result,
                       MdLhs const& cur_node_lhs);
    void CollectRefinersForViolated(MdNode& cur_node, std::vector<MdRefiner>& found,
                                    MdLhs& cur_node_lhs,
                                    PairComparisonResult const& pair_comparison_result,
                                    model::Index cur_node_index);

    bool IsUnsupported(MdLhs const& lhs) const;

    bool IsUnsupportedReplace(LhsSpecialization const& lhs_specialization) const;
    bool IsUnsupportedNonReplace(LhsSpecialization const& lhs_specialization) const;

    void UpdateMaxLevel(LhsSpecialization const& lhs_specialization, auto handle_tail);
    void AddNewMinimal(MdNode& cur_node, MdSpecialization const& md, MdLhs::iterator cur_node_iter,
                       auto handle_level_update_tail);
    MdNode* TryGetNextNode(GeneralizationHelper& helper, model::Index child_array_index,
                           auto new_minimal_action, ColumnClassifierValueId const next_lhs_ccv_id,
                           MdLhs::iterator iter, std::size_t gen_check_offset = 0);

    MdNode* TryGetNextNodeChildMap(MdCCVIdChildMap& child_map, GeneralizationHelper& helper,
                                   model::Index child_array_index, auto new_minimal_action,
                                   ColumnClassifierValueId const next_lhs_ccv_id,
                                   MdLhs::iterator iter, auto get_child_map_iter,
                                   std::size_t gen_check_offset = 0);

    void AddIfMinimal(MdSpecialization const& md, auto handle_tail, auto gen_checker_method);
    void AddIfMinimalAppend(MdSpecialization const& md);
    void WalkToTail(MdSpecialization const& md, GeneralizationHelper& helper,
                    MdLhs::iterator next_lhs_iter, auto handle_level_update_tail);
    void AddIfMinimalReplace(MdSpecialization const& md);
    void AddIfMinimalInsert(MdSpecialization const& md);

    static auto SetUnsupAction() noexcept {
        return [](SupportNode* node) { node->is_unsupported = true; };
    }

    // Generalization check, specialization (add if minimal)
    void MarkNewLhs(SupportNode& cur_node, MdLhs const& lhs, MdLhs::iterator cur_lhs_iter);
    void MarkUnsupported(MdLhs const& lhs);

    void SpecializeElement(MdLhs const& lhs, Rhss const& rhss, MdLhs::iterator lhs_iter,
                           model::Index spec_child_index, ColumnClassifierValueId spec_past,
                           model::Index lhs_spec_index, auto add_method, auto support_check_method);
    void Specialize(MdLhs const& lhs, Rhss const& rhss, auto get_lhs_ccv_id,
                    auto get_nonlhs_ccv_id);
    void Specialize(MdLhs const& lhs, PairComparisonResult const& pair_comparison_result,
                    Rhss const& rhss);
    void Specialize(MdLhs const& lhs, Rhss const& rhss);

    void GetAll(MdNode& cur_node, std::vector<MdLatticeNodeInfo>& collected, MdLhs& cur_node_lhs,
                model::Index this_node_index);

public:
    explicit MdLattice(SingleLevelFunc single_level_func,
                       std::vector<std::vector<ColumnClassifierValueId>> const& lhs_ids,
                       bool prune_nondisjoint, std::size_t max_cardinality, Rhs max_rhs);

    std::size_t GetColMatchNumber() const noexcept {
        return column_matches_size_;
    }

    [[nodiscard]] std::size_t GetMaxLevel() const noexcept {
        return max_level_;
    }

    std::vector<ColumnClassifierValueId> GetInterestingnessCCVIds(
            MdLhs const& lhs, std::vector<model::Index> const& indices) const;
    std::vector<MdVerificationMessenger> GetLevel(std::size_t level);
    std::vector<MdRefiner> CollectRefinersForViolated(
            PairComparisonResult const& pair_comparison_result);
    std::vector<MdLatticeNodeInfo> GetAll();
};

}  // namespace algos::hymd::lattice
