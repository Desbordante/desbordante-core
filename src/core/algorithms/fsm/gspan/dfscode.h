#pragma once

#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <sstream>
#include <stack>
#include <vector>

#include "extended_edge.h"

namespace gspan {

class DFSCode {
    int rightmost_;
    std::vector<int> rightmost_path_;
    std::vector<ExtendedEdge> extended_edges_;

public:
    DFSCode() {
        rightmost_ = -1;
    }

    bool NotPreOfRM(int vertex) const {
        if (rightmost_path_.size() <= 1) return true;
        return vertex != rightmost_path_[rightmost_path_.size() - 2];
    }

    std::vector<int> GetAllLabels() const {
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
        if (extended_edges_.empty()) {
            rightmost_ = 1;
            rightmost_path_.push_back(0);
            rightmost_path_.push_back(1);
        } else {
            int id1 = edge.vertex1.id;
            int id2 = edge.vertex2.id;
            if (id1 < id2) {
                rightmost_ = id2;
                while (!rightmost_path_.empty() && rightmost_path_.back() > id1) {
                    rightmost_path_.pop_back();
                }
                rightmost_path_.push_back(id2);
            }
        }
        extended_edges_.push_back(edge);
    }

    ExtendedEdge const& operator[](size_t i) const {
        return extended_edges_[i];
    }

    ExtendedEdge& operator[](size_t i) {
        return extended_edges_[i];
    }

    bool OnRightMostPath(int vertex_id) const {
        return std::ranges::find(rightmost_path_, vertex_id) != rightmost_path_.end();
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

    int GetRightMost() const {
        return rightmost_;
    }

    std::vector<int> const& GetRightMostPath() const {
        return rightmost_path_;
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
        size_t operator()(DFSCode const& code) const {
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