#include "qvo_strategies.h"

namespace algos {

std::size_t BfsQvoStrategy::GetDomainSize(VertexT pattern_vertex) const {
    auto const it = domain_.find(pattern_vertex);
    if (it == domain_.end()) {
        return 0;
    }
    return it->second.size();
}

std::size_t BfsQvoStrategy::GetPatternDegree(VertexT vertex) const {
    return boost::out_degree(vertex, pattern_) + boost::in_degree(vertex, pattern_);
}

bool BfsQvoStrategy::IsBetterRoot(VertexT lhs, VertexT rhs) const {
    std::size_t const lhs_domain_size = GetDomainSize(lhs);
    std::size_t const rhs_domain_size = GetDomainSize(rhs);

    if (lhs_domain_size != rhs_domain_size) {
        return lhs_domain_size < rhs_domain_size;
    }

    std::size_t const lhs_degree = GetPatternDegree(lhs);
    std::size_t const rhs_degree = GetPatternDegree(rhs);

    if (lhs_degree != rhs_degree) {
        return lhs_degree > rhs_degree;
    }

    return pattern_[lhs].id < pattern_[rhs].id;
}

BfsQvoStrategy::VertexT BfsQvoStrategy::ChooseNextRoot() const {
    bool found = false;
    VertexT best{};

    for (auto const vertex : boost::make_iterator_range(boost::vertices(pattern_))) {
        if (visited_.contains(vertex)) {
            continue;
        }

        if (!found || IsBetterRoot(vertex, best)) {
            best = vertex;
            found = true;
        }
    }

    if (!found) {
        throw std::logic_error("BuildQueryVertexOrder called with no unvisited vertices left");
    }

    return best;
}

void BfsQvoStrategy::EnqueueIfNeeded(VertexT vertex) {
    if (!visited_.contains(vertex) && queued_.emplace(vertex).second) {
        queue_.push_back(vertex);
    }
}

void BfsQvoStrategy::EnqueueUndirectedNeighbors(VertexT vertex) {
    for (auto const edge : boost::make_iterator_range(boost::out_edges(vertex, pattern_))) {
        EnqueueIfNeeded(boost::target(edge, pattern_));
    }

    for (auto const edge : boost::make_iterator_range(boost::in_edges(vertex, pattern_))) {
        EnqueueIfNeeded(boost::source(edge, pattern_));
    }
}

void BfsQvoStrategy::VisitNext() {
    VertexT const current = queue_.front();
    queue_.pop_front();
    queued_.erase(current);

    if (!visited_.emplace(current).second) {
        return;
    }

    qvo_.push_back(current);
    EnqueueUndirectedNeighbors(current);
}

std::vector<BfsQvoStrategy::VertexT> BfsQvoStrategy::Order() {
    qvo_.clear();
    visited_.clear();
    queued_.clear();
    queue_.clear();

    std::size_t const vertex_count = boost::num_vertices(pattern_);
    qvo_.reserve(vertex_count);

    while (qvo_.size() != vertex_count) {
        if (queue_.empty()) {
            EnqueueIfNeeded(ChooseNextRoot());
        }

        VisitNext();
    }

    return qvo_;
}

}  // namespace algos
