#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include <boost/functional/hash.hpp>

#include "core/algorithms/cfd/model/cfd_types.h"

namespace algos::cfd {

class GCGrowth {
public:
    struct Generator {
        Itemset items;
        std::size_t support;
    };

    using ClosedToFree = std::unordered_map<Itemset, std::vector<Generator>, boost::hash<Itemset>>;

private:
    struct Node {
        Node(Item item, Node const* parent) : item(item), parent(parent) {}

        Item item;
        Node const* parent;
        std::size_t support = 0;
        std::size_t terminal_count = 0;
        std::unordered_map<Item, std::unique_ptr<Node>> children;
    };

    using GeneratorToSupport = std::unordered_map<Itemset, std::size_t, boost::hash<Itemset>>;
    using MatchingNodes = std::vector<Node const*>;
    using Extensions = std::map<std::size_t, MatchingNodes>;

    std::unique_ptr<Node> root_;
    std::unordered_map<Item, std::vector<Node const*>> headers_;
    std::unordered_map<Item, std::size_t> item_to_index_;
    GeneratorToSupport gen_to_supp_;
    ClosedToFree closed_to_free_;
    std::vector<Item> items_;
    std::size_t min_support_;
    std::size_t max_lhs_;

    void BuildTree(std::vector<Transaction> const& transactions);
    Itemset GetPath(Node const* node) const;
    Extensions FindExtensions(MatchingNodes const& matching_nodes, std::size_t extension_end) const;
    Itemset ComputeClosure(MatchingNodes const& prefixes) const;
    bool IsGenerator(Itemset const& candidate, std::size_t support) const;
    void MineGenerators();

public:
    ClosedToFree Mine(std::vector<Transaction> const& transaction, std::size_t min_support,
                      std::size_t max_lhs);
};

}  // namespace algos::cfd
