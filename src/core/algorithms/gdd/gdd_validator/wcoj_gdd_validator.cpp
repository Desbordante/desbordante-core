#include "wcoj_gdd_validator.h"

#include <algorithm>
#include <deque>
#include <ranges>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/range/iterator_range.hpp>

namespace algos {

namespace {

// boost::breadth_first_search goes through out_edges only, and there is no
// way to pass directed graph as undirected without making a copy of it (as far as I know)
class BfsQueryVertexOrder {
private:
    using GraphT = model::gdd::graph_t;
    using VertexT = model::gdd::vertex_t;
    using DomainT = std::unordered_map<VertexT, std::vector<VertexT>>;

    GraphT const& pattern_;
    DomainT const& domain_;

    std::vector<VertexT> qvo_;
    std::unordered_set<VertexT> visited_;
    std::unordered_set<VertexT> queued_;
    std::deque<VertexT> queue_;

    std::size_t GetDomainSize(VertexT pattern_vertex) const {
        auto const it = domain_.find(pattern_vertex);
        if (it == domain_.end()) {
            return 0;
        }
        return it->second.size();
    }

    std::size_t GetPatternDegree(VertexT vertex) const {
        return boost::out_degree(vertex, pattern_) + boost::in_degree(vertex, pattern_);
    }

    bool IsBetterRoot(VertexT lhs, VertexT rhs) const {
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

    VertexT ChooseNextRoot() const {
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

    void EnqueueIfNeeded(VertexT vertex) {
        if (!visited_.contains(vertex) && queued_.emplace(vertex).second) {
            queue_.push_back(vertex);
        }
    }

    void EnqueueUndirectedNeighbors(VertexT vertex) {
        for (auto const edge : boost::make_iterator_range(boost::out_edges(vertex, pattern_))) {
            EnqueueIfNeeded(boost::target(edge, pattern_));
        }

        for (auto const edge : boost::make_iterator_range(boost::in_edges(vertex, pattern_))) {
            EnqueueIfNeeded(boost::source(edge, pattern_));
        }
    }

    void VisitNext() {
        VertexT const current = queue_.front();
        queue_.pop_front();
        queued_.erase(current);

        if (!visited_.emplace(current).second) {
            return;
        }

        qvo_.push_back(current);
        EnqueueUndirectedNeighbors(current);
    }

public:
    BfsQueryVertexOrder(GraphT const& pattern, DomainT const& domain)
        : pattern_(pattern), domain_(domain) {}

    std::vector<VertexT> Build() {
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
};

}  // namespace

void WcojGddValidator::Prepare(model::Gdd const& gdd, model::gdd::graph_t const& graph) {
    gdd_ = &gdd;
    graph_ = &graph;
    pattern_ = &gdd.GetPattern();

    match_levels_.clear();
    adjacency_index_.clear();

    match_levels_.reserve(boost::num_vertices(*pattern_));

    domain_ = BuildDomain(*pattern_, *graph_);
    for (auto& set : domain_ | std::views::values) {
        std::ranges::sort(set);
    }
    qvo_ = BuildQueryVertexOrder();
}

std::vector<GddValidator::VertexT> WcojGddValidator::BuildQueryVertexOrder() const {
    // base implementation, does not use graph statistics and does not compute
    // cost of QVO plan
    return BfsQueryVertexOrder(*pattern_, domain_).Build();
}

std::optional<model::GddCounterexample> WcojGddValidator::Holds(model::Gdd const& gdd,
                                                                model::gdd::graph_t const& graph) {
    Prepare(gdd, graph);

    OperationResult result = Scan();
    if (result == OperationResult::kEmpty) {
        return std::nullopt;
    }

    while (result != OperationResult::kFinished) {
        if (result = ExtendIntersect(); result == OperationResult::kEmpty) {
            return std::nullopt;
        }
    }

    for (MappingT const& full_match : match_levels_.back()) {
        if (!gdd_->Satisfies(graph, full_match)) {
            return model::BuildCounterexample(gdd_->GetPattern(), *graph_, full_match);
        }
    }

    return std::nullopt;
}

WcojGddValidator::OperationResult WcojGddValidator::Scan() {
    if (qvo_.empty()) {
        return OperationResult::kEmpty;
    }

    auto const first_pv = qvo_.front();

    auto const domain_it = domain_.find(first_pv);
    if (domain_it == domain_.end() || domain_it->second.empty()) {
        return OperationResult::kEmpty;
    }

    auto const& candidates = domain_it->second;

    MatchLevelT level;
    for (auto const gv : candidates) {
        level.emplace_back(MappingT{{first_pv, gv}});
    }

    match_levels_.emplace_back(std::move(level));
    return OperationResult::kProduced;
}

WcojGddValidator::OperationResult WcojGddValidator::ExtendIntersect() {
    std::size_t const match_level = match_levels_.size();

    if (match_level == 0) {
        throw std::logic_error("Scan call is required before ExtendIntersect call");
    }
    if (match_level >= qvo_.size()) {
        return OperationResult::kFinished;
    }

    auto const new_pv = qvo_[match_level];
    MatchLevelT const& prev_level = match_levels_[match_level - 1];
    MatchLevelT next_level;

    auto const& descriptors = BuildDescriptorsFor(new_pv, match_level);
    for (MappingT const& partial_match : prev_level) {
        std::vector<VertexT> const extension_set =
                ComputeExtensionSet(partial_match, new_pv, descriptors);

        for (VertexT graph_vertex : extension_set) {
            MappingT extended_match = partial_match;

            if (auto const [_, inserted] = extended_match.emplace(new_pv, graph_vertex);
                !inserted) {
                throw std::logic_error("New pattern vertex is already present in partial match");
            }

            next_level.emplace_back(std::move(extended_match));
        }
    }

    if (next_level.empty()) {
        return OperationResult::kEmpty;
    }

    match_levels_.emplace_back(std::move(next_level));

    return match_levels_.size() == qvo_.size() ? OperationResult::kFinished
                                               : OperationResult::kProduced;
}

std::vector<WcojGddValidator::AdjacencyDescriptor> WcojGddValidator::BuildDescriptorsFor(
        VertexT new_pv, std::size_t level) const {
    if (level > qvo_.size()) {
        throw std::logic_error("BuildDescriptorsFor level is greater than qvo_.size()");
    }

    std::vector<AdjacencyDescriptor> descriptors;
    for (std::size_t i = 0; i < level; ++i) {
        VertexT const old_pv = qvo_[i];

        for (auto const edge : boost::make_iterator_range(boost::out_edges(old_pv, *pattern_))) {
            if (boost::target(edge, *pattern_) == new_pv) {
                descriptors.push_back(AdjacencyDescriptor{
                        .pattern_vertex = old_pv,
                        .direction = Direction::kOut,
                        .edge_label = (*pattern_)[edge].label,
                });
            }
        }

        for (auto const edge : boost::make_iterator_range(boost::in_edges(old_pv, *pattern_))) {
            if (boost::source(edge, *pattern_) == new_pv) {
                descriptors.push_back(AdjacencyDescriptor{
                        .pattern_vertex = old_pv,
                        .direction = Direction::kIn,
                        .edge_label = (*pattern_)[edge].label,
                });
            }
        }
    }

    return descriptors;
}

std::vector<GddValidator::VertexT> const& WcojGddValidator::GetNeighbors(
        VertexT graph_vertex, Direction direction, std::string const& edge_label) {
    if (graph_ == nullptr) {
        throw std::logic_error("GetNeighbors called before Prepare");
    }

    NeighborKey const key{
            .graph_vertex = graph_vertex,
            .direction = direction,
            .edge_label = edge_label,
    };

    if (auto const it = adjacency_index_.find(key); it != adjacency_index_.end()) {
        return it->second;
    }

    auto [it, inserted] = adjacency_index_.emplace(key, std::vector<VertexT>{});
    std::vector<VertexT>& neighbors = it->second;

    if (direction == Direction::kOut) {
        for (auto const edge :
             boost::make_iterator_range(boost::out_edges(graph_vertex, *graph_))) {
            if (LabelsMatch(edge_label, (*graph_)[edge].label)) {
                neighbors.push_back(boost::target(edge, *graph_));
            }
        }
    } else {
        for (auto const edge : boost::make_iterator_range(boost::in_edges(graph_vertex, *graph_))) {
            if (LabelsMatch(edge_label, (*graph_)[edge].label)) {
                neighbors.push_back(boost::source(edge, *graph_));
            }
        }
    }

    std::ranges::sort(neighbors);
    auto const [first, last] = std::ranges::unique(neighbors.begin(), neighbors.end());
    neighbors.erase(first, last);
    return neighbors;
}

std::vector<GddValidator::VertexT> WcojGddValidator::ComputeExtensionSet(
        MappingT const& partial_match, VertexT new_pv,
        std::vector<AdjacencyDescriptor> const& descriptors) {
    auto const domain_it = domain_.find(new_pv);
    if (domain_it == domain_.end() || domain_it->second.empty()) {
        return {};
    }

    std::vector<VertexT> const& domain = domain_it->second;

    if (descriptors.empty()) {
        return domain;
    }

    std::vector<std::vector<VertexT> const*> lists;
    lists.reserve(descriptors.size() + 1);
    lists.push_back(&domain);

    for (auto const& [pattern_vertex, direction, edge_label] : descriptors) {
        auto const mapped_it = partial_match.find(pattern_vertex);
        if (mapped_it == partial_match.end()) {
            throw std::logic_error(
                    "Descriptor references a pattern vertex absent from partial match");
        }

        std::vector<VertexT> const& neighbors =
                GetNeighbors(mapped_it->second, direction, edge_label);

        if (neighbors.empty()) {
            return {};
        }

        // neighbors are cached in adjacency_index_, so the pointer lives
        lists.push_back(&neighbors);
    }

    // for faster intersections, |A \cap B| <= max(|A|, |B|)
    std::ranges::sort(lists, [](auto const* a, auto const* b) { return a->size() < b->size(); });

    std::vector<VertexT> acc = IntersectSorted(*lists[0], *lists[1]);
    for (std::size_t i = 2; i < lists.size() && !acc.empty(); ++i) {
        acc = IntersectSorted(acc, *lists[i]);
    }
    return acc;
}

std::vector<GddValidator::VertexT> WcojGddValidator::IntersectSorted(
        std::vector<VertexT> const& lhs, std::vector<VertexT> const& rhs) {
    std::vector<VertexT> result;
    result.reserve(std::min(lhs.size(), rhs.size()));
    std::ranges::set_intersection(lhs, rhs, std::back_inserter(result));
    return result;
}

}  // namespace algos
