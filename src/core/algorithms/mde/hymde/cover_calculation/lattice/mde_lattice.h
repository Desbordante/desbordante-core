#pragma once

#include "algorithms/mde/hymde/cover_calculation/invalidated_rhss.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/mde_node_location.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/single_level_func.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/support_node.h"
#include "algorithms/mde/hymde/cover_calculation/pair_comparison_result.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_lr_map.h"
#include "model/index.h"

namespace algos::hymde::cover_calculation::lattice {
class MdeLattice {
private:
    using MdeRCVIdChildMap = MdeNode::OrderedRCVIdChildMap;
    using MdNodeChildren = MdeNode::Children;

public:
    // TODO: flyweight.
    class PairUpdater {
        MdeLattice* lattice_;
        PairComparisonResult const* pair_comparison_result_;
        MdeNodeLocation node_location_;
        InvalidatedRhss invalidated_;

    public:
        PairUpdater(MdeLattice* lattice, PairComparisonResult const* pair_comparison_result,
                    MdeNodeLocation node_info, InvalidatedRhss invalidated)
            : lattice_(lattice),
              pair_comparison_result_(pair_comparison_result),
              node_location_(std::move(node_info)),
              invalidated_(std::move(invalidated)) {}

        MdeLhs const& GetLhs() const {
            return node_location_.lhs;
        }

        std::size_t Refine();

        bool InvalidatedNumber() const noexcept {
            return invalidated_.Size();
        }
    };

    // TODO: flyweight.
    class ValidationUpdater {
        MdeLattice* lattice_;
        MdeNodeLocation node_location_;

    public:
        ValidationUpdater(MdeLattice* lattice, MdeNodeLocation node_location)
            : lattice_(lattice), node_location_(std::move(node_location)) {}

        MdeLhs const& GetLhs() const {
            return node_location_.lhs;
        }

        Rhs& GetRhs() {
            return node_location_.node->rhs;
        }

        MdeNode const& GetNode() const {
            return *node_location_.node;
        }

        void MarkUnsupported();

        void LowerAndSpecialize(InvalidatedRhss const& invalidated);
    };

    struct PathElement {
        MdeNode* node_ptr;
        MdeRCVIdChildMap* map_ptr;
        MdeRCVIdChildMap::iterator map_it;
    };

private:
    std::size_t max_level_ = 0;
    std::size_t const record_matches_size_;
    MdeNode mde_root_;
    SupportNode support_root_;
    // Is there a way to define a level in such a way that one cannot use each RCV ID independently
    // to determine an MDE's level but the lattice traversal algorithm still works?
    SingleLevelFunc const get_element_level_;
    std::vector<record_match_indexes::RcvIdLRMap> const* const rcv_id_lr_maps_;
    bool const prune_nondisjoint_;
    std::size_t const cardinality_limit_;
    boost::dynamic_bitset<> enabled_rhs_indices_;

    [[nodiscard]] bool HasGeneralization(Mde const& mde) const;
    void ExcludeGeneralizations(MultiMde& mde) const;

    void GetLevel(MdeNode& cur_node, std::vector<ValidationUpdater>& collected,
                  MdeLhs& cur_node_lhs, model::Index cur_node_record_match_index,
                  std::size_t level_left);

    void RaiseInterestingnessRCVIds(
            MdeNode const& cur_node, MdeLhs const& lhs,
            std::vector<RecordClassifierValueId>& cur_interestingness_rcv_ids,
            MdeLhs::iterator cur_lhs_iter, std::vector<model::Index> const& indices,
            std::vector<RecordClassifierValueId> const& rcv_id_bounds,
            std::size_t& max_count) const;

    void TryAddRefiner(std::vector<PairUpdater>& found, MdeNode& cur_node,
                       PairComparisonResult const& pair_comparison_result,
                       MdeLhs const& cur_node_lhs);
    void CollectRefinersForViolated(MdeNode& cur_node, std::vector<PairUpdater>& found,
                                    MdeLhs& cur_node_lhs, MdeLhs::iterator cur_lhs_iter,
                                    PairComparisonResult const& pair_comparison_result);

    bool IsUnsupported(MdeLhs const& lhs) const;

    static auto MarkUnsupportedAction() noexcept {
        return [](SupportNode* node) { node->MarkUnsupported(); };
    }

    // Generalization check, specialization (add if minimal)
    void MarkNewLhs(SupportNode& cur_node, MdeLhs const& lhs, MdeLhs::iterator cur_lhs_iter);
    void MarkUnsupported(MdeLhs const& lhs);

    template <bool MayNotExist>
    void TryDeleteEmptyNode(MdeLhs const& lhs);

    template <typename MdeInfoType>
    auto CreateSpecializer(MdeLhs const& lhs, auto&& rhs, auto get_lhs_rcv_id,
                           auto get_nonlhs_rcv_id);
    void Specialize(MdeLhs const& lhs, Rhss const& rhss, auto get_lhs_rcv_id,
                    auto get_nonlhs_rcv_id);
    void Specialize(MdeLhs const& lhs, PairComparisonResult const& pair_comparison_result,
                    Rhss const& rhss);
    void Specialize(MdeLhs const& lhs, Rhss const& rhss);

    void GetAll(MdeNode& cur_node, MdeLhs& cur_node_lhs, auto&& add_node);

public:
    explicit MdeLattice(SingleLevelFunc single_level_func,
                        std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps,
                        bool prune_nondisjoint, std::size_t max_cardinality, Rhs max_rhs);

    std::size_t GetRecMatchNumber() const noexcept {
        return record_matches_size_;
    }

    [[nodiscard]] std::size_t GetMaxLevel() const noexcept {
        return max_level_;
    }

    std::vector<RecordClassifierValueId> GetInterestingnessRCVIds(
            MdeLhs const& lhs, std::vector<model::Index> const& indices,
            std::vector<RecordClassifierValueId> const& rcv_id_bounds) const;
    // TODO: collect validation selectors immediately.
    std::vector<ValidationUpdater> GetLevel(std::size_t level);
    std::vector<PairUpdater> CollectRefinersForViolated(
            PairComparisonResult const& pair_comparison_result);
    std::vector<MdeNodeLocation> GetAll();
};
}  // namespace algos::hymde::cover_calculation::lattice
