#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include "itemset_node.h"

namespace algos::cind {
class Itemset {
public:
    Itemset(std::unordered_map<Item, std::list<BasketInfo>> items, size_t included_baskets_cnt,
            double min_completeness)
        : root_(std::make_shared<ItemsetNode>(ItemsetNode{nullptr, {}, {}, 0, 0})),
          included_baskets_cnt_(included_baskets_cnt),
          min_completeness_(min_completeness) {
        prev_layer_data_.push_back(root_);
        for (auto& [item, baskets_info] : items) {
            TryEmplace(root_, std::move(item), std::move(baskets_info));
        }
    }

    void CreateNewLayer(
            std::vector<std::tuple<std::shared_ptr<ItemsetNode>, Item, std::list<BasketInfo>>>
                    new_items_info) {
        std::swap(prev_layer_data_, last_layer_data_);
        last_layer_data_.clear();
        for (auto [parent, value, baskets] : new_items_info) {
            TryEmplace(parent, std::move(value), std::move(baskets));
        }

        Cleanup();
    }

    void Cleanup() {
        for (auto parent_node : prev_layer_data_) {
            if (parent_node->GetChildNodes().empty()) {
                parent_node->Cleanup();
            }
        }
    }

    void TryEmplace(std::shared_ptr<ItemsetNode> parent, Item value,
                    std::list<BasketInfo> baskets_info) {
        if (auto child_ptr = parent->CreateChild(std::move(value), std::move(baskets_info),
                                                 included_baskets_cnt_, min_completeness_);
            child_ptr != nullptr) {
            last_layer_data_.push_back(child_ptr);
        }
    }

    bool CheckSubsets(std::vector<Item> const& candidate) {
        if (!root_->CheckSubsetItem(candidate, 0, 1, false)) {
            return false;
        }
        return root_->CheckSubsetItem(candidate, 0, 0, true);
    }

    std::vector<std::shared_ptr<ItemsetNode>> const& GetPrevItems() const noexcept {
        return prev_layer_data_;
    }

    std::vector<std::shared_ptr<ItemsetNode>> const& GetItems() const noexcept {
        return last_layer_data_;
    }

private:
    std::shared_ptr<ItemsetNode> root_;
    size_t included_baskets_cnt_;
    double min_completeness_;
    std::vector<std::shared_ptr<ItemsetNode>> prev_layer_data_;
    std::vector<std::shared_ptr<ItemsetNode>> last_layer_data_;

    friend std::hash<algos::cind::Itemset>;
};
}  // namespace algos::cind
