#include "wcoj_gdd_validator.h"

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/range/iterator_range.hpp>

namespace algos {

void WcojGddValidator::Prepare(model::Gdd const& gdd, model::gdd::graph_t const& graph) {
    gdd_ = &gdd;
    graph_ = &graph;
    pattern_ = &gdd.GetPattern();

    cur_level_.clear();
    next_level_.clear();
    level_count_ = 0;
    adjacency_index_.clear();

    domain_ = BuildDomain(*pattern_, *graph_);
    for (auto& set : domain_ | std::views::values) {
        std::ranges::sort(set);
    }

    if (IsPatternWeaklyConnected()) {
        qvo_ = BuildQueryVertexOrder<CostBasedQvoStrategy>();
    } else {
        qvo_ = BuildQueryVertexOrder<BfsQvoStrategy>();
    }
}

bool WcojGddValidator::IsPatternWeaklyConnected() const {
    if (pattern_ == nullptr) {
        return false;
    }

    std::size_t const n = boost::num_vertices(*pattern_);
    if (n == 0) {
        return true;
    }

    using UndirectedGraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS>;
    UndirectedGraph undirected(n);
    for (auto const e : boost::make_iterator_range(boost::edges(*pattern_))) {
        boost::add_edge(boost::source(e, *pattern_), boost::target(e, *pattern_), undirected);
    }

    std::vector<int> component(n);
    return boost::connected_components(undirected, component.data()) == 1;
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

    std::size_t const count = cur_level_.count();
    std::size_t const width = cur_level_.width;
    MappingT full_match;
    for (std::size_t i = 0; i < count; ++i) {
        VertexT const* row = cur_level_.row(i);
        full_match.clear();
        for (std::size_t j = 0; j < width; ++j) {
            full_match.emplace(qvo_[j], row[j]);
        }

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

    cur_level_.reset(1);
    cur_level_.data.assign(candidates.begin(), candidates.end());

    level_count_ = 1;
    return OperationResult::kProduced;
}

WcojGddValidator::OperationResult WcojGddValidator::ExtendIntersect() {
    intersection_cache_.last_isect_valid = false;

    if (level_count_ == 0) {
        throw std::logic_error("Scan call is required before ExtendIntersect call");
    }
    if (level_count_ >= qvo_.size()) {
        return OperationResult::kFinished;
    }

    auto const new_pv = qvo_[level_count_];
    std::size_t const parent_width = cur_level_.width;
    std::size_t const parent_count = cur_level_.count();

    next_level_.reset(level_count_ + 1);
    // at least one child per parent
    next_level_.data.reserve(cur_level_.data.size() + parent_count);

    auto const& descriptors = BuildDescriptorsFor(new_pv, level_count_);
    for (std::size_t i = 0; i < parent_count; ++i) {
        VertexT const* parent = cur_level_.row(i);
        std::vector<VertexT> const& extension_set =
                ComputeExtensionSet(parent, new_pv, descriptors);

        for (VertexT const graph_vertex : extension_set) {
            next_level_.push_row(parent, parent_width, graph_vertex);
        }
    }

    if (next_level_.count() == 0) {
        return OperationResult::kEmpty;
    }

    std::swap(cur_level_, next_level_);
    ++level_count_;

    return level_count_ == qvo_.size() ? OperationResult::kFinished : OperationResult::kProduced;
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
                        .qvo_index = i,
                        .direction = Direction::kOut,
                        .edge_label = (*pattern_)[edge].label,
                });
            }
        }

        for (auto const edge : boost::make_iterator_range(boost::in_edges(old_pv, *pattern_))) {
            if (boost::source(edge, *pattern_) == new_pv) {
                descriptors.push_back(AdjacencyDescriptor{
                        .pattern_vertex = old_pv,
                        .qvo_index = i,
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

std::vector<GddValidator::VertexT> const& WcojGddValidator::ComputeExtensionSet(
        VertexT const* partial_match, VertexT new_pv,
        std::vector<AdjacencyDescriptor> const& descriptors) {
    auto const domain_it = domain_.find(new_pv);
    if (domain_it == domain_.end() || domain_it->second.empty()) {
        intersection_cache_.last_isect_valid = false;
        intersection_cache_.last_isect_key.clear();
        intersection_cache_.last_isect_set.clear();
        return intersection_cache_.last_isect_set;
    }

    std::vector<VertexT> const& domain = domain_it->second;

    if (descriptors.empty()) {
        intersection_cache_.last_isect_valid = false;
        return domain;
    }

    // lookup intersection cache
    std::vector<VertexT> key;
    key.reserve(descriptors.size());
    for (auto const& d : descriptors) {
        key.push_back(partial_match[d.qvo_index]);
    }

    if (intersection_cache_.last_isect_valid && intersection_cache_.last_isect_key == key) {
        return intersection_cache_.last_isect_set;
    }

    std::vector<std::vector<VertexT> const*> lists;
    lists.reserve(descriptors.size() + 1);
    lists.push_back(&domain);

    for (auto const& d : descriptors) {
        VertexT const mapped = partial_match[d.qvo_index];
        std::vector<VertexT> const& neighbors = GetNeighbors(mapped, d.direction, d.edge_label);

        if (neighbors.empty()) {
            intersection_cache_.last_isect_valid = true;
            intersection_cache_.last_isect_key.assign(key.begin(), key.end());
            intersection_cache_.last_isect_set.clear();
            return intersection_cache_.last_isect_set;
        }

        // neighbors are cached in adjacency_index_, so the pointer lives
        lists.push_back(&neighbors);
    }

    intersection_cache_.last_isect_key.assign(key.begin(), key.end());
    IntersectSorted(lists, intersection_cache_.last_isect_set);
    intersection_cache_.last_isect_valid = true;
    return intersection_cache_.last_isect_set;
}

void WcojGddValidator::IntersectSorted(std::vector<std::vector<VertexT> const*>& lists,
                                       std::vector<VertexT>& out) {
    // for faster intersections, |A \cap B| <= max(|A|, |B|)
    std::ranges::sort(lists, [](auto const* a, auto const* b) { return a->size() < b->size(); });

    out.clear();
    std::ranges::set_intersection(*lists[0], *lists[1], std::back_inserter(out));

    if (lists.size() == 2) {
        return;
    }

    scratch_.clear();
    std::vector<VertexT>* cur = &out;
    std::vector<VertexT>* tmp = &scratch_;

    for (std::size_t i = 2; i < lists.size() && !cur->empty(); ++i) {
        tmp->clear();
        std::ranges::set_intersection(*cur, *lists[i], std::back_inserter(*tmp));
        std::swap(cur, tmp);
    }

    if (cur != &out) {
        std::swap(out, *cur);
    }
}

}  // namespace algos
