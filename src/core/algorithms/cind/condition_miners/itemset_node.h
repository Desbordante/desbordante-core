#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "item.h"

namespace algos::cind {
using BasketInfo = std::tuple<size_t, std::vector<size_t>, bool>;

class ItemsetNode : public std::enable_shared_from_this<ItemsetNode> {
public:
    ItemsetNode(std::shared_ptr<ItemsetNode> parent, Item value,
                std::vector<BasketInfo> baskets_info, double validity, double completeness)
        : value_(std::move(value)),
          baskets_info_(std::move(baskets_info)),
          validity_(validity),
          completeness_(completeness),
          parent_node_(std::move(parent)) {}

    std::shared_ptr<ItemsetNode> CreateChild(Item value, std::vector<BasketInfo> baskets_info,
                                             size_t included_baskets_cnt, double min_completeness) {
        double included_contained_baskets_cnt = 0.0;
        for (auto const& [_, real_id, is_included] : baskets_info) {
            included_contained_baskets_cnt += is_included;
        }

        // avoid nan/inf if included_baskets_cnt == 0
        double const completeness = (included_baskets_cnt == 0)
                                            ? 0.0
                                            : included_contained_baskets_cnt /
                                                      static_cast<double>(included_baskets_cnt);

        if (completeness >= min_completeness) {
            double const validity = baskets_info.empty()
                                            ? -1.0
                                            : included_contained_baskets_cnt /
                                                      static_cast<double>(baskets_info.size());

            auto new_element =
                    std::make_shared<ItemsetNode>(shared_from_this(), std::move(value),
                                                  std::move(baskets_info), validity, completeness);

            child_nodes_.emplace(new_element->GetValue(), new_element);
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

    std::unordered_map<Item, std::shared_ptr<ItemsetNode>> const& GetChildNodes() const noexcept {
        return child_nodes_;
    }

    std::vector<BasketInfo> const& GetBaskets() const noexcept {
        return baskets_info_;
    }

    void Erase(std::shared_ptr<ItemsetNode> child) {
        child_nodes_.erase(child->GetValue());
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
    std::vector<BasketInfo> baskets_info_;
    double validity_;
    double completeness_;
    std::unordered_map<Item, std::shared_ptr<ItemsetNode>> child_nodes_;
    std::shared_ptr<ItemsetNode> parent_node_;
};
}  // namespace algos::cind
