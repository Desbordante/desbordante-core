#pragma once

#include "algorithms/md/hymd/lattice/lhs_specialization.h"
#include "algorithms/md/hymd/lattice/total_generalization_checker.h"

namespace algos::hymd::lattice {
template <typename NodeType>
class SpecGeneralizationChecker {
    using Specialization = NodeType::Specialization;
    using CCVIdChildMap = NodeType::OrderedCCVIdChildMap;

    Specialization const& specialization_;
    TotalGeneralizationChecker<NodeType> total_checker_{specialization_.ToUnspecialized()};

    bool HasGeneralizationTotal(NodeType const& node, MdLhs::iterator iter,
                                model::Index child_array_index) const {
        return total_checker_.HasGeneralization(node, iter, child_array_index);
    }

    bool HasChildGenSpec(NodeType const& node, model::Index child_array_index,
                         MdLhs::iterator fol_iter, ColumnClassifierValueId ccv_id_limit,
                         model::Index next_child_array_index, auto gen_method,
                         auto get_child_map_iter) const {
        CCVIdChildMap const& child_map = *node.children[child_array_index];
        for (auto spec_iter = get_child_map_iter(child_map), end_iter = child_map.end();
             spec_iter != end_iter; ++spec_iter) {
            auto const& [generalization_ccv_id, node] = *spec_iter;
            if (generalization_ccv_id > ccv_id_limit) break;
            if ((this->*gen_method)(node, fol_iter, next_child_array_index)) return true;
        }
        return false;
    }

    static auto GetFirstAction() noexcept {
        return [](CCVIdChildMap const& child_map) { return child_map.begin(); };
    }

    bool HasGeneralizationInChildren(NodeType const& node, MdLhs::iterator next_node_iter,
                                     model::Index child_array_index, auto spec_method,
                                     auto final_method) const {
        LhsSpecialization const& lhs_specialization = specialization_.GetLhsSpecialization();
        MdLhs::iterator spec_iter = lhs_specialization.specialization_data.spec_before;
        while (next_node_iter != spec_iter) {
            auto const& [delta, next_ccv_id] = *next_node_iter;
            ++next_node_iter;
            child_array_index += delta;
            if (HasChildGenSpec(node, child_array_index, next_node_iter, next_ccv_id, 0,
                                spec_method, GetFirstAction()))
                return true;
            ++child_array_index;
        }
        auto const& [spec_delta, spec_ccv_id] = lhs_specialization.specialization_data.new_child;
        child_array_index += spec_delta;
        return (this->*final_method)(node, child_array_index, spec_iter, spec_ccv_id, spec_delta);
    }

    bool ReplaceFinalCheck(NodeType const& node, model::Index child_array_index,
                           MdLhs::iterator spec_iter, ColumnClassifierValueId ccv_id,
                           std::size_t spec_delta) const {
        assert(spec_iter != specialization_.GetLhsSpecialization().old_lhs.end() &&
               spec_iter->child_array_index == spec_delta);
        ColumnClassifierValueId const old_ccv_id = spec_iter->ccv_id;
        auto get_higher = [&](CCVIdChildMap const& child_map) {
            return child_map.upper_bound(old_ccv_id);
        };
        ++spec_iter;
        return HasChildGenSpec(node, child_array_index, spec_iter, ccv_id, 0,
                               &SpecGeneralizationChecker::HasGeneralizationTotal, get_higher);
    }

    bool NonReplaceFinalCheck(NodeType const& node, model::Index child_array_index,
                              MdLhs::iterator spec_iter, ColumnClassifierValueId ccv_id,
                              std::size_t spec_delta) const {
        assert(spec_iter == specialization_.GetLhsSpecialization().old_lhs.end() ||
               spec_iter->child_array_index > spec_delta);
        return HasChildGenSpec(node, child_array_index, spec_iter, ccv_id, -(spec_delta + 1),
                               &SpecGeneralizationChecker::HasGeneralizationTotal,
                               GetFirstAction());
    }

public:
    SpecGeneralizationChecker(Specialization const& specialization)
        : specialization_(specialization) {}

    bool HasGeneralizationInChildrenReplace(NodeType const& node, MdLhs::iterator next_node_iter,
                                            model::Index child_array_index = 0) const {
        return HasGeneralizationInChildren(
                node, next_node_iter, child_array_index,
                &SpecGeneralizationChecker::HasGeneralizationInChildrenReplace,
                &SpecGeneralizationChecker::ReplaceFinalCheck);
    }

    bool HasGeneralizationInChildrenNonReplace(NodeType const& node, MdLhs::iterator next_node_iter,
                                               model::Index child_array_index = 0) const {
        return HasGeneralizationInChildren(
                node, next_node_iter, child_array_index,
                &SpecGeneralizationChecker::HasGeneralizationInChildrenNonReplace,
                &SpecGeneralizationChecker::NonReplaceFinalCheck);
    }

    bool HasGeneralizationReplace(NodeType const& node, MdLhs::iterator iter) const {
        return HasGeneralizationInChildrenReplace(node, iter);
    }

    bool HasGeneralizationNonReplace(NodeType const& node, MdLhs::iterator iter) const {
        return HasGeneralizationInChildrenNonReplace(node, iter);
    }

    bool HasGeneralizationReplace(NodeType const& node) const {
        return HasGeneralizationReplace(node,
                                        specialization_.GetLhsSpecialization().old_lhs.begin());
    }

    bool HasGeneralizationNonReplace(NodeType const& node) const {
        return HasGeneralizationNonReplace(node,
                                           specialization_.GetLhsSpecialization().old_lhs.begin());
    }

    auto const& GetTotalChecker() const noexcept {
        return total_checker_;
    }
};
}  // namespace algos::hymd::lattice
