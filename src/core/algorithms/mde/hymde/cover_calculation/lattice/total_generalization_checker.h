#pragma once

#include <type_traits>

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "model/index.h"

namespace algos::hymde::cover_calculation::lattice {
template <typename NodeType, typename Unspecialized = NodeType::Specialization::Unspecialized>
class TotalGeneralizationChecker {
    using RCVIdChildMap = NodeType::OrderedRCVIdChildMap;
    using RCVIdChildMapIterator = RCVIdChildMap::const_iterator;
    using FasterType = typename std::remove_cvref_t<Unspecialized>::FasterType;

    Unspecialized unspecialized_;

public:
    TotalGeneralizationChecker(Unspecialized unspecialized) noexcept
        : unspecialized_(unspecialized) {}

    bool HasGeneralizationInRCVIdMap(MdeLhs::iterator next_iter, RecordClassifierValueId lhs_rcv_id,
                                     RCVIdChildMap const& map, RCVIdChildMapIterator map_iter) {
        for (RCVIdChildMapIterator map_end = map.end(); map_iter != map_end; ++map_iter) {
            auto const& [child_rcv_id, node] = *map_iter;
            if (child_rcv_id > lhs_rcv_id) break;
            if (HasGeneralization(node, next_iter)) return true;
        }
        return false;
    }

    bool HasGeneralizationInRCVIdMap(MdeLhs::iterator next_iter, RecordClassifierValueId lhs_rcv_id,
                                     RCVIdChildMap const& map) {
        return HasGeneralizationInRCVIdMap(next_iter, lhs_rcv_id, map, map.begin());
    }

    bool HasGeneralizationInChildren(NodeType const& node, MdeLhs::iterator next_iter,
                                     model::Index total_offset = 0) {
        MdeLhs const& lhs = NodeType::GetLhs(unspecialized_);
        for (MdeLhs::iterator end_iter = lhs.end(); next_iter != end_iter; ++total_offset) {
            auto const& [next_node_offset, lhs_rcv_id] = *next_iter;
            total_offset += next_node_offset;
            ++next_iter;
            RCVIdChildMap const& map = node.children[total_offset];
            if (HasGeneralizationInRCVIdMap(next_iter, lhs_rcv_id, map)) return true;
        }
        return false;
    }

    bool HasGeneralization(NodeType const& node, MdeLhs::iterator next_iter,
                           model::Index starting_offset = 0) {
        if (CheckNode(node)) return true;
        // TODO: try switching from MultiMd to Md if only one RHS is left
        return HasGeneralizationInChildren(node, next_iter, starting_offset);
    }

    bool HasGeneralization(NodeType const& node) {
        return HasGeneralization(node, NodeType::GetLhs(unspecialized_).begin());
    }

    bool CheckNode(NodeType const& node) {
        return node.ContainsGeneralizationOf(unspecialized_);
    }

    Unspecialized const& GetUnspecialized() const noexcept {
        return unspecialized_;
    }

    Unspecialized& GetUnspecialized() noexcept {
        return unspecialized_;
    }
};
}  // namespace algos::hymde::cover_calculation::lattice
