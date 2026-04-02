#pragma once

#include <cstddef>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "itemset_node.h"

namespace algos::cind {
class Itemset {
private:
    std::shared_ptr<ItemsetNode> root_;
    std::vector<std::shared_ptr<ItemsetNode>> prev_items_;
    std::vector<std::shared_ptr<ItemsetNode>> items_;

    std::size_t included_baskets_cnt_{0};
    double min_completeness_{0.0};

public:
    using ItemsMap = std::unordered_map<Item, std::vector<BasketInfo>>;

    Itemset(ItemsMap items, std::size_t included_baskets_cnt, double min_completeness)
        : included_baskets_cnt_(included_baskets_cnt), min_completeness_(min_completeness) {
        root_ = std::make_shared<ItemsetNode>(nullptr, Item{}, std::vector<BasketInfo>{}, -1.0,
                                              0.0);
        prev_items_.clear();
        prev_items_.push_back(root_);

        items_.clear();
        items_.reserve(items.size());

        for (auto& [item, baskets] : items) {
            if (auto child = root_->CreateChild(item, std::move(baskets), included_baskets_cnt_,
                                                min_completeness_)) {
                items_.push_back(std::move(child));
            }
        }
    }

    std::vector<std::shared_ptr<ItemsetNode>> const& GetItems() const noexcept {
        return items_;
    }

    std::vector<std::shared_ptr<ItemsetNode>> const& GetPrevItems() const noexcept {
        return prev_items_;
    }

    bool CheckSubsets(std::vector<Item> const& candidate) const {
        // Check all (|candidate|-1)-subsets exist in trie
        if (candidate.size() <= 1) {
            return true;
        }

        for (std::size_t skip = 0; skip < candidate.size(); ++skip) {
            auto node = root_;
            for (std::size_t i = 0; i < candidate.size(); ++i) {
                if (i == skip) {
                    continue;
                }
                auto const& children = node->GetChildNodes();
                auto it = children.find(candidate[i]);
                if (it == children.end()) {
                    return false;
                }
                node = it->second;
            }
        }
        return true;
    }

    void CreateNewLayer(
            std::vector<std::tuple<std::shared_ptr<ItemsetNode>, Item, std::vector<BasketInfo>>>
                    new_items_info) {
        items_.clear();
        prev_items_.clear();
        items_.reserve(new_items_info.size());

        std::unordered_set<ItemsetNode*> seen_parents;
        seen_parents.reserve(new_items_info.size());

        for (auto& [parent, value, baskets] : new_items_info) {
            auto child = parent->CreateChild(std::move(value), std::move(baskets),
                                             included_baskets_cnt_, min_completeness_);
            if (!child) {
                continue;
            }

            items_.push_back(child);

            if (seen_parents.insert(parent.get()).second) {
                prev_items_.push_back(parent);
            }
        }
    }
};
}  // namespace algos::cind
