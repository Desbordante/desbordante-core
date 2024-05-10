#pragma once

#include <vector>

#include <boost/dynamic_bitset.hpp>

// see algorithms/ucc/hpivalid/LICENSE

namespace algos::hpiv {

typedef boost::dynamic_bitset<> Edge;
typedef boost::dynamic_bitset<> Edgemark;

class Hypergraph {
private:
    Edge::size_type num_vertices_;
    std::vector<Edge> edges_;

public:
    Hypergraph() = delete;
    explicit Hypergraph(Edge::size_type num_vertices);

    std::vector<Edge>::size_type NumEdges() const {
        return edges_.size();
    }

    Edge::size_type NumVertices() const {
        return num_vertices_;
    }

    void AddEdge(Edge const& new_edge) {
        edges_.push_back(new_edge);
    }

    void AddEdge(Edge&& new_edge) {
        edges_.push_back(new_edge);
    }

    void RemoveLastEdge() {
        edges_.pop_back();
    }

    void AddEdgeAndMinimizeInclusion(Edge const& new_edge);

    // operators and related

    Edge& operator[](std::vector<Edge>::size_type i_e) {
        return edges_[i_e];
    }

    Edge const& operator[](std::vector<Edge>::size_type i_e) const {
        return edges_[i_e];
    }

    // NOLINTBEGIN(readability-identifier-naming)
    std::vector<Edge>::iterator begin() {
        return edges_.begin();
    }

    std::vector<Edge>::const_iterator begin() const {
        return edges_.begin();
    }

    std::vector<Edge>::iterator end() {
        return edges_.end();
    }

    std::vector<Edge>::const_iterator end() const {
        return edges_.end();
    }

    // NOLINTEND(readability-identifier-naming)
};

}  // namespace algos::hpiv
