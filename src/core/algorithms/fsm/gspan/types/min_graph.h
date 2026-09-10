#pragma once

#include <algorithm>
#include <vector>

#include "dfscode.h"

namespace gspan {

struct MinEdge {
    int from;
    int label;
    int to;
    int id;
};

struct MinVertex {
    int id = 0;
    int label = 0;
    std::vector<MinEdge> edges;
};

class MinGraph {
    std::vector<MinVertex> vertices_;
    int num_edges_ = 0;

public:
    void BuildFromDFSCode(DFSCode const& code) {
        int max_id = 0;
        for (auto const& ee : code.GetExtendedEdges()) {
            max_id = std::max({max_id, ee.vertex1.id, ee.vertex2.id});
        }

        for (auto& v : vertices_) {
            v.edges.clear();
        }
        vertices_.resize(max_id + 1);

        int edge_id = 0;
        for (auto const& ee : code.GetExtendedEdges()) {
            vertices_[ee.vertex1.id].id = ee.vertex1.id;
            vertices_[ee.vertex1.id].label = ee.vertex1.label;
            vertices_[ee.vertex2.id].id = ee.vertex2.id;
            vertices_[ee.vertex2.id].label = ee.vertex2.label;
            vertices_[ee.vertex1.id].edges.push_back(
                    {ee.vertex1.id, ee.label, ee.vertex2.id, edge_id});
            vertices_[ee.vertex2.id].edges.push_back(
                    {ee.vertex2.id, ee.label, ee.vertex1.id, edge_id});
            edge_id++;
        }
        num_edges_ = edge_id;
    }

    MinVertex const& operator[](size_t i) const {
        return vertices_[i];
    }

    size_t NumVertices() const {
        return vertices_.size();
    }

    int NumEdges() const {
        return num_edges_;
    }

    auto begin() const {
        return vertices_.begin();
    }

    auto end() const {
        return vertices_.end();
    }
};

}  // namespace gspan