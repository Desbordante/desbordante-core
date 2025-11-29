#pragma once

#include <type_traits>

#include "core/algorithms/md/hymd/md_element.h"
#include "core/algorithms/md/hymd/md_lhs.h"
#include "core/model/index.h"

namespace algos::hymd::lattice {
template <typename NodeType, typename Unspecialized = NodeType::Specialization::Unspecialized>
class TotalGeneralizationChecker {
    using CCVIdChildMap = NodeType::OrderedCCVIdChildMap;
    using CCVIdChildMapIterator = CCVIdChildMap::const_iterator;
    using FasterType = typename std::remove_cvref_t<Unspecialized>::FasterType;

    Unspecialized unspecialized_;

public:
    TotalGeneralizationChecker(Unspecialized unspecialized) noexcept
        : unspecialized_(unspecialized) {}

    bool HasGeneralizationInCCVIdMap(MdLhs::iterator next_iter, ColumnClassifierValueId lhs_ccv_id,
                                     CCVIdChildMap const& map, CCVIdChildMapIterator map_iter) {
        CCVIdChildMapIterator map_end = map.end();
        for (; map_iter != map_end; ++map_iter) {
            auto const& [child_ccv_id, node] = *map_iter;
            if (child_ccv_id > lhs_ccv_id) break;
            if (HasGeneralization(node, next_iter)) return true;
        }
        return false;
    }

    bool HasGeneralizationInCCVIdMap(MdLhs::iterator next_iter, ColumnClassifierValueId lhs_ccv_id,
                                     CCVIdChildMap const& map) {
        return HasGeneralizationInCCVIdMap(next_iter, lhs_ccv_id, map, map.begin());
    }

    bool HasGeneralizationInChildren(NodeType const& node, MdLhs::iterator next_iter,
                                     model::Index total_offset = 0) {
        MdLhs const& lhs = NodeType::GetLhs(unspecialized_);
        for (MdLhs::iterator end_iter = lhs.end(); next_iter != end_iter; ++total_offset) {
            auto const& [next_node_offset, lhs_ccv_id] = *next_iter;
            total_offset += next_node_offset;
            ++next_iter;
            CCVIdChildMap const& map = node.children[total_offset];
            if (HasGeneralizationInCCVIdMap(next_iter, lhs_ccv_id, map)) return true;
        }
        return false;
    }

    bool HasGeneralization(NodeType const& node, MdLhs::iterator next_iter,
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
}  // namespace algos::hymd::lattice
