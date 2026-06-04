#pragma once

#include <deque>
#include <vector>

#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"

namespace algos {

template <typename T>
concept QvoStrategy = requires(T strategy) {
    requires std::constructible_from<T, model::gdd::graph_t const& /*graph*/,
                                     model::gdd::graph_t const& /*pattern*/,
                                     model::gdd::detail::DomainT const&> /*domain*/;

    { strategy.Order() } -> std::convertible_to<std::vector<model::gdd::vertex_t>>;
};

class BfsQvoStrategy {
private:
    using GraphT = model::gdd::graph_t;
    using VertexT = model::gdd::vertex_t;
    using DomainT = model::gdd::detail::DomainT;

    GraphT const& pattern_;
    DomainT const& domain_;

    std::vector<VertexT> qvo_;
    std::unordered_set<VertexT> visited_;
    std::unordered_set<VertexT> queued_;
    std::deque<VertexT> queue_;

    std::size_t GetDomainSize(VertexT pattern_vertex) const;
    std::size_t GetPatternDegree(VertexT vertex) const;
    bool IsBetterRoot(VertexT lhs, VertexT rhs) const;
    VertexT ChooseNextRoot() const;
    void EnqueueIfNeeded(VertexT vertex);
    void EnqueueUndirectedNeighbors(VertexT vertex);
    void VisitNext();

public:
    BfsQvoStrategy(GraphT const& /*graph*/, GraphT const& pattern, DomainT const& domain)
        : pattern_(pattern), domain_(domain) {}

    std::vector<VertexT> Order();
};

class CostBasedQvoStrategy {
private:
    using GraphT = model::gdd::graph_t;
    using VertexT = model::gdd::vertex_t;
    using DomainT = model::gdd::detail::DomainT;

    enum class Direction : std::uint8_t { kIn, kOut };

    struct ExtensionListDescriptor {
        VertexT from;  // placed in order pattern vertex
        Direction direction;
        std::string edge_label;
    };

    static constexpr std::size_t kExhaustiveLimit = 7;

    GraphT const& graph_;
    GraphT const& pattern_;
    DomainT const& domain_;

    std::unordered_map<std::string, double> avg_out_size_;
    std::unordered_map<std::string, double> avg_in_size_;
    void ComputeAvgListSizes();

    std::size_t VertexDomainSize(VertexT pattern_vertex) const;
    double AvgListSize(std::string const& edge_label, Direction direction) const;

    bool ConnectsToPlaced(VertexT vertex,
                          std::unordered_map<VertexT, std::size_t> const& placed) const;
    std::vector<ExtensionListDescriptor> ExtensionListsFor(
            VertexT to, std::unordered_map<VertexT, std::size_t> const& placed) const;
    double StepCost(VertexT next, std::size_t level,
                    std::unordered_map<VertexT, std::size_t> const& placed,
                    std::vector<double> const& card_by_level, double& match_estimate) const;

    std::vector<VertexT> OrderExhaustive() const;
    std::vector<VertexT> OrderGreedy() const;

    struct PatternVerticesIdComparator {
        GraphT const& pattern;

        bool operator()(VertexT a, VertexT b) const noexcept {
            return pattern[a].id < pattern[b].id;
        }
    } pattern_vertices_comparator_{pattern_};

public:
    CostBasedQvoStrategy(GraphT const& graph, GraphT const& pattern, DomainT const& domain)
        : graph_(graph), pattern_(pattern), domain_(domain) {
        ComputeAvgListSizes();
    }

    std::vector<VertexT> Order() const {
        std::size_t const m = boost::num_vertices(pattern_);
        if (m == 0) {
            return {};
        }
        return m <= kExhaustiveLimit ? OrderExhaustive() : OrderGreedy();
    }
};

}  // namespace algos
