#pragma once

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/md/hymd/lattice/md.h"
#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/lattice/md_node.h"
#include "algorithms/md/hymd/lattice/node_base.h"
#include "algorithms/md/hymd/lattice/rhs.h"
#include "algorithms/md/hymd/lattice/single_level_func.h"
#include "algorithms/md/hymd/lattice/support_node.h"
#include "algorithms/md/hymd/lhs_ccv_ids_info.h"
#include "algorithms/md/hymd/md_element.h"
#include "algorithms/md/hymd/md_lhs.h"
#include "algorithms/md/hymd/pair_comparison_result.h"
#include "algorithms/md/hymd/rhss.h"
#include "algorithms/md/hymd/utility/invalidated_rhss.h"
#include "md/hymd/column_classifier_value_id.h"
#include "model/index.h"

namespace algos {
namespace hymd {
namespace lattice {
struct Md;
struct MultiMd;
}  // namespace lattice
struct LhsCCVIdsInfo;
struct PairComparisonResult;
}  // namespace hymd
}  // namespace algos

namespace algos::hymd::lattice {

class MdLattice {
private:
    using MdCCVIdChildMap = MdNode::OrderedCCVIdChildMap;
    using MdNodeChildren = MdNode::Children;

public:
    class MdRefiner {
        MdLattice* lattice_;
        PairComparisonResult const* pair_comparison_result_;
        MdLatticeNodeInfo node_info_;
        utility::InvalidatedRhss invalidated_;

    public:
        MdRefiner(MdLattice* lattice, PairComparisonResult const* pair_comparison_result,
                  MdLatticeNodeInfo node_info, utility::InvalidatedRhss invalidated)
            : lattice_(lattice),
              pair_comparison_result_(pair_comparison_result),
              node_info_(std::move(node_info)),
              invalidated_(std::move(invalidated)) {}

        MdLhs const& GetLhs() const {
            return node_info_.lhs;
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
            return node_info_.node->rhs;
        }

        MdNode const& GetNode() const {
            return *node_info_.node;
        }

        void MarkUnsupported();

        void LowerAndSpecialize(utility::InvalidatedRhss const& invalidated);
    };

    struct PathInfo {
        MdNode* node_ptr;
        MdCCVIdChildMap* map_ptr;
        MdCCVIdChildMap::iterator map_it;
    };

private:
    std::size_t max_level_ = 0;
    std::size_t const column_matches_size_;
    MdNode md_root_;
    SupportNode support_root_;
    // Is there a way to define a level in such a way that one cannot use each CCV ID independently
    // to determine an MD's level but the lattice traversal algorithm still works?
    SingleLevelFunc const get_element_level_;
    std::vector<LhsCCVIdsInfo> const* const lhs_ccv_id_info_;
    bool const prune_nondisjoint_;
    std::size_t const cardinality_limit_;
    boost::dynamic_bitset<> enabled_rhs_indices_;

    [[nodiscard]] bool HasGeneralization(Md const& md) const;
    void ExcludeGeneralizations(MultiMd& md) const;

    void GetLevel(MdNode& cur_node, std::vector<MdVerificationMessenger>& collected,
                  MdLhs& cur_node_lhs, model::Index cur_node_column_match_index,
                  std::size_t level_left);

    void RaiseInterestingnessCCVIds(
            MdNode const& cur_node, MdLhs const& lhs,
            std::vector<ColumnClassifierValueId>& cur_interestingness_ccv_ids,
            MdLhs::iterator cur_lhs_iter, std::vector<model::Index> const& indices,
            std::vector<ColumnClassifierValueId> const& ccv_id_bounds,
            std::size_t& max_count) const;

    void TryAddRefiner(std::vector<MdRefiner>& found, MdNode& cur_node,
                       PairComparisonResult const& pair_comparison_result,
                       MdLhs const& cur_node_lhs);
    void CollectRefinersForViolated(MdNode& cur_node, std::vector<MdRefiner>& found,
                                    MdLhs& cur_node_lhs, MdLhs::iterator cur_lhs_iter,
                                    PairComparisonResult const& pair_comparison_result);

    bool IsUnsupported(MdLhs const& lhs) const;

    static auto SetUnsupAction() noexcept {
        return [](SupportNode* node) { node->is_unsupported = true; };
    }

    // Generalization check, specialization (add if minimal)
    void MarkNewLhs(SupportNode& cur_node, MdLhs const& lhs, MdLhs::iterator cur_lhs_iter);
    void MarkUnsupported(MdLhs const& lhs);

    template <bool MayNotExist>
    void TryDeleteEmptyNode(MdLhs const& lhs);

    template <typename MdInfoType>
    auto CreateSpecializer(MdLhs const& lhs, auto&& rhs, auto get_lhs_ccv_id,
                           auto get_nonlhs_ccv_id);
    void Specialize(MdLhs const& lhs, Rhss const& rhss, auto get_lhs_ccv_id,
                    auto get_nonlhs_ccv_id);
    void Specialize(MdLhs const& lhs, PairComparisonResult const& pair_comparison_result,
                    Rhss const& rhss);
    void Specialize(MdLhs const& lhs, Rhss const& rhss);

    void GetAll(MdNode& cur_node, MdLhs& cur_node_lhs, auto&& add_node);

public:
    explicit MdLattice(SingleLevelFunc single_level_func,
                       std::vector<LhsCCVIdsInfo> const& lhs_ccv_ids_info, bool prune_nondisjoint,
                       std::size_t max_cardinality, Rhs max_rhs);

    std::size_t GetColMatchNumber() const noexcept {
        return column_matches_size_;
    }

    [[nodiscard]] std::size_t GetMaxLevel() const noexcept {
        return max_level_;
    }

    std::vector<ColumnClassifierValueId> GetInterestingnessCCVIds(
            MdLhs const& lhs, std::vector<model::Index> const& indices,
            std::vector<ColumnClassifierValueId> const& ccv_id_bounds) const;
    std::vector<MdVerificationMessenger> GetLevel(std::size_t level);
    std::vector<MdRefiner> CollectRefinersForViolated(
            PairComparisonResult const& pair_comparison_result);
    std::vector<MdLatticeNodeInfo> GetAll();
};

}  // namespace algos::hymd::lattice
