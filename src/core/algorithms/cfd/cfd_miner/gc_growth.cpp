#include "gc_growth.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace algos::cfd {

GCGrowth::ClosedToFree GCGrowth::Mine(std::vector<Transaction> const& transactions,
                                      std::size_t min_support, std::size_t max_lhs) {
    if (min_support == 0) {
        throw std::invalid_argument("Minimum support must be greater than zero.");
    }

    headers_.clear();
    root_.reset();
    item_to_index_.clear();
    gen_to_supp_.clear();
    closed_to_free_.clear();
    items_.clear();
    min_support_ = min_support;
    max_lhs_ = max_lhs;

    if (transactions.size() < min_support) {
        return {};
    }

    BuildTree(transactions);
    gen_to_supp_.emplace(Itemset{}, transactions.size());
    closed_to_free_[ComputeClosure({root_.get()})].push_back({{}, transactions.size()});
    MineGenerators();

    return std::move(closed_to_free_);
}

void GCGrowth::BuildTree(std::vector<Transaction> const& transactions) {
    std::unordered_map<Item, std::size_t> frequencies;
    for (auto row : transactions) {
        std::sort(row.begin(), row.end());
        row.erase(std::unique(row.begin(), row.end()), row.end());
        for (Item item : row) {
            ++frequencies[item];
        }
    }

    for (auto const& [item, support] : frequencies) {
        if (support >= min_support_) {
            items_.push_back(item);
        }
    }
    std::sort(items_.begin(), items_.end());
    for (std::size_t index = 0; index < items_.size(); ++index) {
        item_to_index_.emplace(items_[index], index);
    }

    root_ = std::make_unique<Node>(0, nullptr);
    for (auto row : transactions) {
        std::sort(row.begin(), row.end());
        row.erase(std::unique(row.begin(), row.end()), row.end());
        Node* current = root_.get();
        ++current->support;
        for (Item item : row) {
            if (frequencies.at(item) < min_support_) {
                continue;
            }

            auto child = current->children.find(item);
            if (child == current->children.end()) {
                auto new_child = std::make_unique<Node>(item, current);
                Node* next = new_child.get();
                current->children.emplace(item, std::move(new_child));
                headers_[item].push_back(next);
                current = next;
            } else {
                current = child->second.get();
            }
            ++current->support;
        }
        ++current->terminal_count;
    }
}

Itemset GCGrowth::GetPath(Node const* node) const {
    Itemset path;
    while (node != root_.get()) {
        path.push_back(node->item);
        node = node->parent;
    }
    std::reverse(path.begin(), path.end());

    return path;
}

GCGrowth::Extensions GCGrowth::FindExtensions(MatchingNodes const& matching_nodes,
                                              std::size_t extension_end) const {
    Extensions extensions;
    for (Node const* matching_node : matching_nodes) {
        for (Node const* ancestor = matching_node->parent; ancestor != root_.get();
             ancestor = ancestor->parent) {
            std::size_t const index = item_to_index_.at(ancestor->item);
            if (index < extension_end) {
                extensions[index].push_back(matching_node);
            }
        }
    }

    return extensions;
}

Itemset GCGrowth::ComputeClosure(std::vector<Node const*> const& tree_nodes) const {
    Itemset closure;
    bool initialized = false;
    auto nodes = tree_nodes;
    while (!nodes.empty()) {
        auto node = nodes.back();
        nodes.pop_back();
        if (node->terminal_count != 0) {
            auto path = GetPath(node);
            if (!initialized) {
                closure = std::move(path);
                initialized = true;
            } else {
                Itemset common;
                std::set_intersection(closure.begin(), closure.end(), path.begin(), path.end(),
                                      std::back_inserter(common));
                closure = std::move(common);
            }
            if (closure.empty()) {
                return closure;
            }
        }

        for (auto const& [item, child] : node->children) {
            nodes.push_back(child.get());
        }
    }

    return closure;
}

bool GCGrowth::IsGenerator(Itemset const& candidate, std::size_t support) const {
    if (support < min_support_) {
        return false;
    }

    for (std::size_t index = 0; index < candidate.size(); ++index) {
        auto subset = candidate;
        subset.erase(subset.begin() + index);
        auto parent = gen_to_supp_.find(subset);
        if (parent == gen_to_supp_.end() || parent->second <= support) {
            return false;
        }
    }

    return true;
}

void GCGrowth::MineGenerators() {
    struct GrowthTask {
        Itemset prefix;
        std::size_t extension_index;
        MatchingNodes matching_nodes;
    };

    std::vector<GrowthTask> tasks;
    tasks.reserve(items_.size());
    for (std::size_t index = items_.size(); index-- != 0;) {
        tasks.push_back({{}, index, std::move(headers_.at(items_[index]))});
    }

    while (!tasks.empty()) {
        GrowthTask task = std::move(tasks.back());
        tasks.pop_back();

        Itemset candidate = std::move(task.prefix);
        candidate.insert(candidate.begin(), items_[task.extension_index]);

        std::size_t support = 0;
        for (Node const* node : task.matching_nodes) {
            support += node->support;
        }
        if (!IsGenerator(candidate, support)) {
            continue;
        }

        gen_to_supp_.emplace(candidate, support);
        closed_to_free_[ComputeClosure(task.matching_nodes)].push_back({candidate, support});

        if (candidate.size() >= max_lhs_) {
            continue;
        }

        auto extensions = FindExtensions(task.matching_nodes, task.extension_index);
        for (auto extension = extensions.rbegin(); extension != extensions.rend(); ++extension) {
            tasks.push_back({candidate, extension->first, std::move(extension->second)});
        }
    }
}

}  // namespace algos::cfd
