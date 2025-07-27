#pragma once

#include <cstddef>

#include "algorithms/mde/hymde/cover_calculation/lattice/lhs_specialization.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/total_generalization_checker.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "model/index.h"

namespace algos::hymde::cover_calculation::lattice {
template <typename NodeType, typename Specialization = NodeType::Specialization>
class SpecGeneralizationChecker {
    using RCVIdChildMap = NodeType::OrderedRCVIdChildMap;

    Specialization& specialization_;
    TotalGeneralizationChecker<NodeType, typename Specialization::Unspecialized> total_checker_{
            specialization_.ToUnspecialized()};

    bool HasGeneralizationTotal(NodeType const& node, MdeLhs::iterator iter,
                                model::Index starting_offset) {
        return total_checker_.HasGeneralization(node, iter, starting_offset);
    }

    bool HasGeneralizationInChildMap(NodeType const& node, model::Index child_array_index,
                                     MdeLhs::iterator next_node_iter,
                                     RecordClassifierValueId rcv_id_limit,
                                     model::Index next_child_array_index, auto gen_method,
                                     auto get_child_map_iter) {
        RCVIdChildMap const& child_map = node.children[child_array_index];
        for (auto spec_iter = get_child_map_iter(child_map), end_iter = child_map.end();
             spec_iter != end_iter; ++spec_iter) {
            auto const& [generalization_rcv_id, node] = *spec_iter;
            if (generalization_rcv_id > rcv_id_limit) break;
            if ((this->*gen_method)(node, next_node_iter, next_child_array_index)) return true;
        }
        return false;
    }

    static auto GetFirstAction() noexcept {
        return [](RCVIdChildMap const& child_map) { return child_map.begin(); };
    }

    bool HasGeneralizationInChildren(NodeType const& node, MdeLhs::iterator next_node_iter,
                                     model::Index total_offset, auto spec_method,
                                     auto final_method) {
        LhsSpecialization const& lhs_specialization = specialization_.GetLhsSpecialization();
        MdeLhs::iterator spec_iter = lhs_specialization.specialization_data.spec_before;
        while (next_node_iter != spec_iter) {
            auto const& [next_node_offset, next_rcv_id] = *next_node_iter;
            ++next_node_iter;
            total_offset += next_node_offset;
            if (HasGeneralizationInChildMap(node, total_offset, next_node_iter, next_rcv_id, 0,
                                            spec_method, GetFirstAction()))
                return true;
            ++total_offset;
        }
        auto const& [new_child_offset, spec_rcv_id] =
                lhs_specialization.specialization_data.new_child;
        total_offset += new_child_offset;
        return (this->*final_method)(node, total_offset, spec_iter, spec_rcv_id, new_child_offset);
    }

    bool ReplaceFinalCheck(NodeType const& node, model::Index replaced_node_offset,
                           MdeLhs::iterator spec_iter, RecordClassifierValueId increased_rcv_id,
                           [[maybe_unused]] std::size_t new_node_offset) {
        assert(spec_iter != specialization_.GetLhsSpecialization().old_lhs.end() &&
               spec_iter->offset == new_node_offset);
        RecordClassifierValueId const old_rcv_id = spec_iter->rcv_id;
        auto get_higher = [&](RCVIdChildMap const& child_map) {
            return child_map.upper_bound(old_rcv_id);
        };
        ++spec_iter;
        return HasGeneralizationInChildMap(node, replaced_node_offset, spec_iter, increased_rcv_id,
                                           0, &SpecGeneralizationChecker::HasGeneralizationTotal,
                                           get_higher);
    }

    bool NonReplaceFinalCheck(NodeType const& node, model::Index inserted_or_appended_node_offset,
                              MdeLhs::iterator spec_iter, RecordClassifierValueId rcv_id,
                              std::size_t new_node_offset) {
        assert(spec_iter == specialization_.GetLhsSpecialization().old_lhs.end() ||
               spec_iter->offset > new_node_offset);
        return HasGeneralizationInChildMap(
                node, inserted_or_appended_node_offset, spec_iter, rcv_id, -(new_node_offset + 1),
                &SpecGeneralizationChecker::HasGeneralizationTotal, GetFirstAction());
    }

public:
    SpecGeneralizationChecker(Specialization& specialization) : specialization_(specialization) {}

    bool HasGeneralizationInChildrenReplace(NodeType const& node, MdeLhs::iterator next_node_iter,
                                            model::Index starting_offset = 0) {
        return HasGeneralizationInChildren(
                node, next_node_iter, starting_offset,
                &SpecGeneralizationChecker::HasGeneralizationInChildrenReplace,
                &SpecGeneralizationChecker::ReplaceFinalCheck);
    }

    bool HasGeneralizationInChildrenNonReplace(NodeType const& node,
                                               MdeLhs::iterator next_node_iter,
                                               model::Index starting_offset = 0) {
        return HasGeneralizationInChildren(
                node, next_node_iter, starting_offset,
                &SpecGeneralizationChecker::HasGeneralizationInChildrenNonReplace,
                &SpecGeneralizationChecker::NonReplaceFinalCheck);
    }

    bool HasGeneralizationReplace(NodeType const& node, MdeLhs::iterator iter) {
        return HasGeneralizationInChildrenReplace(node, iter);
    }

    bool HasGeneralizationNonReplace(NodeType const& node, MdeLhs::iterator iter) {
        return HasGeneralizationInChildrenNonReplace(node, iter);
    }

    bool HasGeneralizationReplace(NodeType const& node) {
        return HasGeneralizationReplace(node,
                                        specialization_.GetLhsSpecialization().old_lhs.begin());
    }

    bool HasGeneralizationNonReplace(NodeType const& node) {
        return HasGeneralizationNonReplace(node,
                                           specialization_.GetLhsSpecialization().old_lhs.begin());
    }

    auto& GetTotalChecker() noexcept {
        return total_checker_;
    }
};
}  // namespace algos::hymde::cover_calculation::lattice
