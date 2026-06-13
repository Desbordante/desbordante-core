#pragma once

#include <map>
#include <ranges>
#include <sstream>
#include <vector>

#include "extended_edge.h"

namespace gspan {

class DFSCode {
    std::vector<ExtendedEdge> extended_edges_;

public:
    std::vector<int> GetVertexLabels() const {
        std::vector<int> result;
        std::unordered_map<int, int> id_to_label;
        for (ExtendedEdge const& ee : extended_edges_) {
            id_to_label[ee.vertex1.id] = ee.vertex1.label;
            id_to_label[ee.vertex2.id] = ee.vertex2.label;
        }

        size_t count = 0;
        while (id_to_label.find(count) != id_to_label.end()) {
            result.push_back(id_to_label[count]);
            count++;
        }

        return result;
    }

    void Add(ExtendedEdge const& edge) {
        extended_edges_.push_back(edge);
    }

    void Pop() {
        extended_edges_.pop_back();
    }

    ExtendedEdge const& operator[](size_t i) const {
        return extended_edges_[i];
    }

    ExtendedEdge& operator[](size_t i) {
        return extended_edges_[i];
    }

    bool ContainEdge(int v1, int v2) const {
        for (ExtendedEdge const& ee : extended_edges_) {
            if ((ee.vertex1.id == v1 && ee.vertex2.id == v2) ||
                (ee.vertex1.id == v2 && ee.vertex2.id == v1))
                return true;
        }
        return false;
    }

    size_t Size() const {
        return extended_edges_.size();
    }

    bool Empty() const {
        return extended_edges_.empty();
    }

    std::vector<ExtendedEdge> const& GetExtendedEdges() const {
        return extended_edges_;
    }

    std::string ToString() const {
        std::stringstream ss;
        ss << "DFSCode: ";
        for (auto const& ee : extended_edges_) {
            ss << ee.ToString() << " ";
        }
        return ss.str();
    }

    bool operator==(DFSCode const& other) const {
        return extended_edges_ == other.extended_edges_;
    }

    bool operator!=(DFSCode const& other) const {
        return !(*this == other);
    }

    struct Hash {
        size_t operator()(DFSCode const& code) const noexcept {
            size_t seed = 0;
            ExtendedEdge::Hash edge_hasher;
            for (auto const& ee : code.extended_edges_) {
                boost::hash_combine(seed, edge_hasher(ee));
            }
            return seed;
        }
    };
};

}  // namespace gspan