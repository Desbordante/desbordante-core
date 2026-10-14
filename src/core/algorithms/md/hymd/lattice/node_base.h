#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <map>
#include <type_traits>
#include <vector>

#include "core/algorithms/md/hymd/column_classifier_value_id.h"
#include "core/algorithms/md/hymd/md_lhs.h"
#include "core/model/index.h"
#include "core/util/desbordante_assume.h"

namespace algos::hymd::lattice {

template <typename NodeType>
class NodeBase {
public:
    using OrderedCCVIdChildMap = std::map<ColumnClassifierValueId, NodeType>;
    using Children = std::vector<OrderedCCVIdChildMap>;

    Children children;

    NodeBase(std::size_t children_number) : children(children_number) {}

    std::size_t GetNextNodeChildArraySize(model::Index next_node_offset) const noexcept {
        return children.size() - (next_node_offset + 1);
    }

    void ForEachNonEmpty(auto action) {
        ForEachNonEmpty(*this, action);
    }

    void ForEachNonEmpty(auto action) const {
        ForEachNonEmpty(*this, action);
    }

    template <typename Self>
    static void ForEachNonEmpty(Self& self, auto action) {
        model::Index index = 0;
        for (std::size_t const array_size = self.children.size(); index != array_size; ++index) {
            using ChildMapType =
                    std::conditional_t<std::is_const_v<Self>, OrderedCCVIdChildMap const,
                                       OrderedCCVIdChildMap>;
            ChildMapType& child_map = self.children[index];
            if (!child_map.empty()) action(child_map, index);
        };
    }

    bool IsEmpty() const noexcept {
        return std::all_of(children.begin(), children.end(),
                           std::mem_fn(&OrderedCCVIdChildMap::empty));
    }

protected:
    template <typename... Args>
    NodeType* AddOneUncheckedBase(model::Index offset, ColumnClassifierValueId ccv_id,
                                  Args... args) {
        return &children[offset]
                        .try_emplace(ccv_id, args..., GetNextNodeChildArraySize(offset))
                        .first->second;
    }
};

template <typename NodeType>
void AddUnchecked(NodeType* cur_node_ptr, MdLhs const& lhs, MdLhs::iterator cur_lhs_iter,
                  auto&& final_node_action, auto&& adder) {
    assert(cur_node_ptr->IsEmpty());
    for (MdLhs::iterator const lhs_end = lhs.end(); cur_lhs_iter != lhs_end; ++cur_lhs_iter) {
        auto const& [next_node_offset, next_ccv_id] = *cur_lhs_iter;
        cur_node_ptr = adder(cur_node_ptr, next_node_offset, next_ccv_id);
    }
    final_node_action(cur_node_ptr);
}

template <typename NodeType>
void AddUnchecked(NodeType* cur_node_ptr, MdLhs const& lhs, MdLhs::iterator cur_lhs_iter,
                  auto&& final_node_action) {
    AddUnchecked(
            cur_node_ptr, lhs, cur_lhs_iter, final_node_action,
            [](NodeType* node, model::Index next_node_offset, ColumnClassifierValueId next_ccv_id) {
                return node->AddOneUnchecked(next_node_offset, next_ccv_id);
            });
}

template <typename NodeType>
void CheckedAdd(NodeType* cur_node_ptr, MdLhs const& lhs, auto const& info, auto&& unchecked_add,
                auto&& final_node_action) {
    for (auto lhs_iter = lhs.begin(), lhs_end = lhs.end(); lhs_iter != lhs_end;) {
        auto const& [next_node_offset, next_ccv_id] = *lhs_iter;
        ++lhs_iter;
        std::size_t const next_child_array_size =
                cur_node_ptr->GetNextNodeChildArraySize(next_node_offset);
        typename NodeType::OrderedCCVIdChildMap& child_map =
                cur_node_ptr->children[next_node_offset];
        if (child_map.empty()) {
            NodeType& new_node =
                    child_map.try_emplace(next_ccv_id, next_child_array_size).first->second;
            unchecked_add(new_node, info, lhs_iter);
            return;
        }
        auto [it_map, is_first_map] = child_map.try_emplace(next_ccv_id, next_child_array_size);
        NodeType& next_node = it_map->second;
        if (is_first_map) {
            unchecked_add(next_node, info, lhs_iter);
            return;
        }
        cur_node_ptr = &next_node;
    };
    final_node_action(cur_node_ptr);
}
}  // namespace algos::hymd::lattice
