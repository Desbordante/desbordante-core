#pragma once

#include <concepts>
#include <deque>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/algorithms/gdd/gdd_graph_description.h"

namespace algos {

template <typename T>
concept QvoStrategy = requires(T strategy) {
    requires std::constructible_from<T, model::gdd::GraphTypes::Graph const& /*graph*/,
                                     model::gdd::GraphTypes::Graph const& /*pattern*/,
                                     model::gdd::GraphTypes::Domain const&> /*domain*/;

    { strategy.Order() } -> std::convertible_to<std::vector<model::gdd::GraphTypes::Vertex>>;
};

class BfsQvoStrategy : private model::gdd::GraphTypes {
private:
    Graph const& pattern_;
    Domain const& domain_;

    std::vector<Vertex> qvo_;
    std::unordered_set<Vertex> visited_;
    std::unordered_set<Vertex> queued_;
    std::deque<Vertex> queue_;

    std::size_t GetDomainSize(Vertex pattern_vertex) const;
    std::size_t GetPatternDegree(Vertex vertex) const;
    bool IsBetterRoot(Vertex lhs, Vertex rhs) const;
    Vertex ChooseNextRoot() const;
    void EnqueueIfNeeded(Vertex vertex);
    void EnqueueUndirectedNeighbors(Vertex vertex);
    void VisitNext();

public:
    BfsQvoStrategy(Graph const& /*graph*/, Graph const& pattern, Domain const& domain)
        : pattern_(pattern), domain_(domain) {}

    std::vector<Vertex> Order();
};

class CostBasedQvoStrategy : private model::gdd::GraphTypes {
private:
    enum class Direction : std::uint8_t { kIn, kOut };

    struct ExtensionListDescriptor {
        Vertex from;  // placed in order pattern vertex
        Direction direction;
        std::string_view edge_label;
    };

    static constexpr std::size_t kExhaustiveLimit = 7;

    Graph const& graph_;
    Graph const& pattern_;
    Domain const& domain_;

    std::unordered_map<std::string_view, double> avg_out_size_;
    std::unordered_map<std::string_view, double> avg_in_size_;
    void ComputeAvgListSizes();

    std::size_t VertexDomainSize(Vertex pattern_vertex) const;
    double AvgListSize(std::string_view edge_label, Direction direction) const;

    bool ConnectsToPlaced(Vertex vertex,
                          std::unordered_map<Vertex, std::size_t> const& placed) const;
    std::vector<ExtensionListDescriptor> ExtensionListsFor(
            Vertex to, std::unordered_map<Vertex, std::size_t> const& placed) const;
    double StepCost(Vertex next, std::size_t level,
                    std::unordered_map<Vertex, std::size_t> const& placed,
                    std::vector<double> const& card_by_level, double& match_estimate) const;

    std::vector<Vertex> OrderExhaustive() const;
    std::vector<Vertex> OrderGreedy() const;

    struct PatternVerticesIdComparator {
        Graph const& pattern;

        bool operator()(Vertex a, Vertex b) const noexcept {
            return pattern[a].id < pattern[b].id;
        }
    } pattern_vertices_comparator_{pattern_};

public:
    CostBasedQvoStrategy(Graph const& graph, Graph const& pattern, Domain const& domain)
        : graph_(graph), pattern_(pattern), domain_(domain) {
        ComputeAvgListSizes();
    }

    std::vector<Vertex> Order() const {
        std::size_t const m = boost::num_vertices(pattern_);
        if (m == 0) {
            return {};
        }
        return m <= kExhaustiveLimit ? OrderExhaustive() : OrderGreedy();
    }
};

}  // namespace algos
