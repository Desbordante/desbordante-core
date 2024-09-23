#pragma once

#include <type_traits>

#include "algorithms/md/hymd/md_element.h"
#include "algorithms/md/hymd/md_lhs.h"
#include "model/index.h"

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
                                     model::Index child_array_index = 0) {
        MdLhs const& lhs = NodeType::GetLhs(unspecialized_);
        for (MdLhs::iterator end_iter = lhs.end(); next_iter != end_iter; ++child_array_index) {
            auto const& [index_delta, lhs_ccv_id] = *next_iter;
            child_array_index += index_delta;
            ++next_iter;
            CCVIdChildMap const& map = *node.children[child_array_index];
            if (HasGeneralizationInCCVIdMap(next_iter, lhs_ccv_id, map)) return true;
        }
        return false;
    }

    bool HasGeneralization(NodeType const& node, MdLhs::iterator next_iter,
                           model::Index child_array_index = 0) {
        if (CheckNode(node)) return true;
        // TODO: try switching from MultiMd to Md if only one RHS is left
        return HasGeneralizationInChildren(node, next_iter, child_array_index);
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
};
}  // namespace algos::hymd::lattice
