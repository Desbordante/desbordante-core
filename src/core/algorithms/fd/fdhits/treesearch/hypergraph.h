#pragma once

#include <algorithm>
#include <vector>

namespace algos::fd::fdhits {
template <typename E>
class Hypergraph {
private:
    size_t vertex_count_;
    std::vector<E> edges_;

public:
    explicit Hypergraph(size_t vertex_count) : vertex_count_(vertex_count) {}

    void AddEdge(E edge) {
        edges_.push_back(std::move(edge));
    }

    void Reserve(size_t size) {
        edges_.reserve(size);
    }

    E const& GetEdge(size_t idx) const {
        return edges_.at(idx);
    }

    size_t GetVertexCount() const {
        return vertex_count_;
    }

    size_t GetEdgeCount() const {
        return edges_.size();
    }

    auto begin() {
        return edges_.begin();
    }

    auto end() {
        return edges_.end();
    }

    auto begin() const {
        return edges_.cbegin();
    }

    auto end() const {
        return edges_.cend();
    }

    void AddEdgeAndMinimize(E const& edge) {
        if (Implies(edge)) return;
        edges_.erase(std::remove_if(edges_.begin(), edges_.end(),
                                    [&edge](E const& e) { return edge.Implies(e); }),
                     edges_.end());
        AddEdge(edge);
    }

    bool Implies(E const& edge) const {
        return std::any_of(edges_.begin(), edges_.end(),
                           [&edge](E const& e) { return e.Implies(edge); });
    }
};

}  // namespace algos::fd::fdhits
