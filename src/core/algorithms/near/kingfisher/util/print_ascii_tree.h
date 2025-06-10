#pragma once
// Debugging utility to visualize CandidatePrefixTree in console.

#include <iostream>
#include <sstream>
#include <string>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/near/kingfisher/node.h"

namespace kingfisher {
// Helper to convert a dynamic_bitset to a human-readable string (MSB first).
std::string BitsetToString(boost::dynamic_bitset<> const& bs) {
    std::string s;
    s.reserve(bs.size());
    for (size_t i = 0; i < bs.size(); ++i) {
        // Print highest-index bit first
        s.push_back(bs[bs.size() - 1 - i] ? '1' : '0');
    }
    return s;
}

// Recursively prints all children of a node in ASCII form, including bitsets when available.
void PrintAsciiTreeChildren(Node const& node, size_t feat_count, std::string const& prefix) {
    std::vector<std::pair<OFeatureIndex, std::shared_ptr<Node>>> childNodes;
    for (OFeatureIndex i = 0; i < feat_count; ++i) {
        if (node.HasChild(i)) {
            childNodes.emplace_back(i, node.children.at(i));
        }
    }

    for (size_t i = 0; i < childNodes.size(); ++i) {
        bool isLast = (i + 1 == childNodes.size());
        std::cout << prefix << (isLast ? "└── " : "├── ");
        std::cout << "[" << childNodes[i].first << "] Node";

        auto bp = childNodes[i].second.get();
        std::cout << " [p:" << BitsetToString(bp->p_possible)
                  << " n:" << BitsetToString(bp->n_possible) << "]";

        std::cout << std::endl;

        std::string childPrefix = prefix + (isLast ? "    " : "│   ");
        PrintAsciiTreeChildren(*childNodes[i].second, feat_count, childPrefix);
    }
}

// Entry point for printing an ASCII tree from the root.
void PrintAsciiTree(Node const& root, size_t feat_count) {
    std::cout << "Node";
    std::cout << " [p:" << BitsetToString(root.p_possible)
              << " n:" << BitsetToString(root.n_possible) << "]";
    std::cout << std::endl;
    PrintAsciiTreeChildren(root, feat_count, "");
}

}  // namespace kingfisher