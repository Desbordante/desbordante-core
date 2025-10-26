#pragma once

#include <algorithm>
#include <list>
#include <memory>
#include <unordered_set>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include "item.h"

namespace algos::cind {
using BasketInfo = std::tuple<size_t, std::list<size_t>, bool>;

class ItemsetNode : public std::enable_shared_from_this<ItemsetNode> {
public:
    ItemsetNode(std::shared_ptr<ItemsetNode> parent, Item value, std::list<BasketInfo> baskets_info,
                double validity, double completeness)
        : value_(std::move(value)),
          baskets_info_(std::move(baskets_info)),
          validity_(validity),
          completeness_(completeness),
          parent_node_(std::move(parent)) {}

    std::shared_ptr<ItemsetNode> CreateChild(Item value, std::list<BasketInfo> baskets_info,
                                             size_t included_baskets_cnt, double min_completeness) {
        double included_contained_baskets_cnt = 0;
        for (auto const& [_, real_id, is_included] : baskets_info) {
            included_contained_baskets_cnt += is_included;
        }
        double completeness = included_contained_baskets_cnt / included_baskets_cnt;
        if (completeness >= min_completeness) {
            double validity = -1;
            if (baskets_info.size()) {
                validity = included_contained_baskets_cnt / baskets_info.size();
            }
            std::shared_ptr<ItemsetNode> new_element =
                    std::make_shared<ItemsetNode>(shared_from_this(), std::move(value),
                                                  std::move(baskets_info), validity, completeness);
            child_nodes_.insert(new_element);
            return new_element;
        }
        return nullptr;
    }

    std::vector<Item> GetContents() const {
        if (parent_node_ == nullptr) {
            return {};
        }
        std::vector<Item> result = parent_node_->GetContents();
        result.emplace_back(value_);
        return result;
    }

    Item const& GetValue() const noexcept {
        return value_;
    }

    double GetValidity() const noexcept {
        return validity_;
    }

    double GetCompleteness() const noexcept {
        return completeness_;
    }

    std::shared_ptr<ItemsetNode> const& GetParent() const noexcept {
        return parent_node_;
    }

    std::unordered_set<std::shared_ptr<ItemsetNode>> const& GetChildNodes() const noexcept {
        return child_nodes_;
    }

    std::list<BasketInfo> const& GetBaskets() const noexcept {
        return baskets_info_;
    }

    bool CheckSubsets(std::vector<Item> const& candidate, size_t curr_idx, bool do_branch) const {
        if (value_ != candidate.at(curr_idx)) {
            return false;
        }
        if (do_branch) {
            if (!CheckSubsetItem(candidate, curr_idx, 2, false)) {
                return false;
            }
        }
        return CheckSubsetItem(candidate, curr_idx, 1, do_branch);
    }

    bool CheckSubsetItem(std::vector<Item> const& candidate, size_t curr_idx, size_t offset,
                         bool do_branch) const {
        if (curr_idx + offset < candidate.size()) {
            auto const& it = std::find_if(child_nodes_.begin(), child_nodes_.end(),
                                          [&](std::shared_ptr<ItemsetNode> const& node) {
                                              return node->GetValue() == candidate.at(curr_idx + 1);
                                          });
            return it != child_nodes_.end() &&
                   it->get()->CheckSubsets(candidate, curr_idx + 1, do_branch);
        }
        return true;
    }

    void Erase(std::shared_ptr<ItemsetNode> child) {
        child_nodes_.erase(child);
        if (child_nodes_.empty()) {
            Cleanup();
        }
    }

    void Cleanup() {
        if (parent_node_ != nullptr) {
            parent_node_->Erase(shared_from_this());
        }
    }

private:
    Item value_;
    std::list<BasketInfo> baskets_info_;
    double validity_;
    double completeness_;
    std::unordered_set<std::shared_ptr<ItemsetNode>> child_nodes_;
    std::shared_ptr<ItemsetNode> parent_node_;
};
}  // namespace algos::cind

template <>
struct std::hash<algos::cind::ItemsetNode> {
    size_t operator()(algos::cind::ItemsetNode const& node) const {
        std::hash<algos::cind::Item> item_hasher;
        return item_hasher(node.GetValue());
    }
};