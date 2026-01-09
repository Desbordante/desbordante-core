#pragma once

#include <set>
#include <sstream>
#include <string>

#include <boost/container_hash/hash.hpp>

#include "dfscode.h"

namespace gspan {

struct FrequentSubgraph {
    size_t id;

    DFSCode dfs_code;

    // The ids of graphs where the subgraph appears
    std::unordered_set<int> graphs_ids;

    int support;

    FrequentSubgraph(size_t id, DFSCode const& dfs_code, std::unordered_set<int> const& graphs_ids,
                     int support)
        : id(id), dfs_code(dfs_code), graphs_ids(graphs_ids), support(support) {}

    int CompareTo(FrequentSubgraph const& other) {
        long dif = support - other.support;
        if (dif > 0) return 1;
        if (dif < 0) return -1;

        return 0;
    }

    std::string ToString() const {
        std::stringstream ss;
        ss << "t # " << id << " * " << support << '\n';
        if (dfs_code.Size() == 1) {
            ExtendedEdge const& ee = dfs_code.GetExtendedEdges()[0];
            if (ee.label == -1) {
                ss << "v 0 " << ee.vertex1.label << '\n';
            } else {
                ss << "v 0 " << ee.vertex1.label << '\n';
                ss << "v 1 " << ee.vertex2.label << '\n';
                ss << "e 0 1 " << ee.label << '\n';
            }
        } else {
            auto labels = dfs_code.GetAllLabels();
            for (size_t j = 0; j < labels.size(); j++) {
                ss << "v " << j << " " << labels[j] << '\n';
            }
            for (auto const& ee : dfs_code.GetExtendedEdges()) {
                ss << "e " << ee.vertex1.id << " " << ee.vertex2.id << " " << ee.label << '\n';
            }
        }

        ss << "x";
        std::vector<int> ids(graphs_ids.begin(), graphs_ids.end());
        std::sort(ids.begin(), ids.end());
        for (auto id : ids) {
            ss << " " << id;
        }
        ss << '\n';

        return ss.str();
    }

    bool operator==(FrequentSubgraph const& other) const {
        return dfs_code == other.dfs_code && support == other.support &&
               graphs_ids == other.graphs_ids;
    }

    bool operator!=(FrequentSubgraph const& other) const = default;

    struct Hash {
        size_t operator()(FrequentSubgraph const& fs) const {
            size_t seed = 0;
            boost::hash_combine(seed, DFSCode::Hash{}(fs.dfs_code));
            boost::hash_combine(seed, fs.support);
            std::vector<int> ids(fs.graphs_ids.begin(), fs.graphs_ids.end());
            std::sort(ids.begin(), ids.end());
            for (int id : ids) {
                boost::hash_combine(seed, id);
            }
            return seed;
        }
    };
};

}  // namespace gspan