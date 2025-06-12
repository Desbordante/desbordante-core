#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/near/kingfisher/node.h"

namespace kingfisher {

std::string BitsetToString(boost::dynamic_bitset<> const& bs) {
    std::string s;
    s.reserve(bs.size());
    for (size_t i = 0; i < bs.size(); ++i) {
        // Print highest-index bit first
        s.push_back(bs[i] ? '1' : '0');
    }
    return s;
}

void ChildrenToString(Node const& node, size_t feat_count, std::string const& prefix, std::ostringstream& out) {
    std::vector<std::pair<OFeatureIndex, std::shared_ptr<Node>>> childNodes;
    for (OFeatureIndex i = 0; i < feat_count; ++i) {
        if (node.HasChild(i)) {
            childNodes.emplace_back(i, node.children.at(i));
        }
    }

    for (size_t i = 0; i < childNodes.size(); ++i) {
        bool isLast = (i + 1 == childNodes.size());
        out << prefix << (isLast ? "└── " : "├── ");
        out << "[" << childNodes[i].first << "] Node";

        auto bp = childNodes[i].second.get();
        out << " [p:" << BitsetToString(bp->p_possible)
            << " n:" << BitsetToString(bp->n_possible) << "]\n";

        std::string childPrefix = prefix + (isLast ? "    " : "│   ");
        ChildrenToString(*childNodes[i].second, feat_count, childPrefix, out);
    }
}

// Debugging utility to visualize CandidatePrefixTree as a string
std::string TreeToString(Node const& root, size_t feat_count) {
    std::ostringstream out;
    out << "Node";
    out << " [p:" << BitsetToString(root.p_possible)
        << " n:" << BitsetToString(root.n_possible) << "]\n";
    ChildrenToString(root, feat_count, "", out);
    return out.str();
}

}  // namespace kingfisher
