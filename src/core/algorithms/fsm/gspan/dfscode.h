#pragma once

#include <map>
#include <ranges>
#include <sstream>
#include <vector>

#include "extended_edge.h"

namespace gspan {

class DFSCode {
    mutable std::vector<int> rightmost_path_;
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

    void Pop(){
        extended_edges_.pop_back();
    }

    void ResetRightmostPath() const {
        rightmost_path_ = {0};
    }

    //   Clears right_most_path, then stores into it the rightmost path of the dfs code
    //   list. The path is stored such that the first item in right_most_path is the
    //   index of the edge 'discovering' the rightmost vertex, the second is the index
    //   of the edge discovering the 'from' vertex of the first edge, and so on.
    //   DFSCode is treated as if it is truncated to the given size.
    void UpdateRightmostPath(size_t size) const {
        rightmost_path_.clear();
        int prev_id = -1;

        // Go in reverse, since we need to first look for the edge that discovered
        // the rightmost vertex
        for (auto i = size; i > 0; --i) {
            // Only consider forward edges (as by definition the rightmost path only
            // consists of edges 'discovering' new nodes). The first forward edge (or
            // equivalently, the last forward edge in DFSCode) is the edge discovering
            // the rightmost vertex. After that, each new edge is the edge discovering
            // the 'from' of the previous one.
            if (extended_edges_[i - 1].vertex1.id < extended_edges_[i - 1].vertex2.id &&
                (rightmost_path_.empty() || prev_id == extended_edges_[i - 1].vertex2.id)) {
                prev_id = extended_edges_[i - 1].vertex1.id;
                rightmost_path_.push_back(i - 1);
            }
        }
    }

    ExtendedEdge const& operator[](size_t i) const {
        return extended_edges_[i];
    }

    ExtendedEdge& operator[](size_t i) {
        return extended_edges_[i];
    }

    bool OnRightMostPath(int vertex_id) const {
        if (vertex_id == 0) return true;
        for (int idx : rightmost_path_) {
            if (extended_edges_[idx].vertex2.id == vertex_id) return true;
        }
        return false;
    }

    ExtendedEdge const& GetEdgeFromRightMostPath(size_t i) const {
        auto id = rightmost_path_[i];
        return extended_edges_[id];
    }

    ExtendedEdge const& GetRightMostEdge() const {
        return extended_edges_[rightmost_path_.front()];
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