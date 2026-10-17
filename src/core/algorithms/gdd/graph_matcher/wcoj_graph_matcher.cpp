#include "wcoj_graph_matcher.h"

#include <algorithm>
#include <cassert>
#include <ranges>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/range/iterator_range.hpp>

namespace algos::detail {

namespace {

// first position in [first, last) holding a value >= target
// complexity is O(log d) where d is the distance to result
template <typename T>
T const* LeapFrogSeek(T const* first, T const* last, T target) {
    if (first == last || !(*first < target)) {
        return first;
    }

    // *first < target holds from here on, so `lo` always stays below the answer
    auto const size = static_cast<std::size_t>(last - first);
    std::size_t lo = 0;
    std::size_t hi = 1;
    while (hi < size && first[hi] < target) {
        lo = hi;
        hi <<= 1;
    }

    return std::lower_bound(first + lo + 1, first + std::min(hi, size), target);
}

}  // namespace

WcojCore::MatchLevel const& WcojCore::FindAll(Graph const& pattern, Domain domain,
                                              MatchSemantics semantics) {
    semantics_ = semantics;
    Prepare(pattern, std::move(domain));

    OperationResult result = Scan();
    while (result == OperationResult::kProduced) {
        result = ExtendIntersect(pattern);
    }

    if (result == OperationResult::kEmpty) {
        cur_level_.Clear();
    }

    return cur_level_;
}

void WcojCore::Prepare(Graph const& pattern, Domain domain) {
    // adjacency_index_ is keyed by (graph vertex, direction, edge label) only, so it
    // stays valid across every pattern matched in the same graph
    cur_level_.Clear();
    next_level_.Clear();
    level_count_ = 0;

    domain_ = std::move(domain);
    for (auto& set : domain_ | std::views::values) {
        std::ranges::sort(set);
    }

    if (IsWeaklyConnected(pattern)) {
        qvo_ = BuildQueryVertexOrder<CostBasedQvoStrategy>(pattern);
    } else {
        qvo_ = BuildQueryVertexOrder<BfsQvoStrategy>(pattern);
    }
}

bool WcojCore::IsWeaklyConnected(Graph const& graph) {
    std::size_t const n = boost::num_vertices(graph);
    if (n == 0) {
        return true;
    }

    using UndirectedGraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS>;
    UndirectedGraph undirected(n);
    for (auto const e : boost::make_iterator_range(boost::edges(graph))) {
        boost::add_edge(boost::source(e, graph), boost::target(e, graph), undirected);
    }

    std::vector<int> component(n);
    return boost::connected_components(undirected, component.data()) == 1;
}

WcojCore::OperationResult WcojCore::Scan() {
    assert(!qvo_.empty());

    auto const first_pv = qvo_.front();
    auto const domain_it = domain_.find(first_pv);
    if (domain_it == domain_.end() || domain_it->second.empty()) {
        return OperationResult::kEmpty;
    }

    auto const& candidates = domain_it->second;

    cur_level_.Reset(1);
    cur_level_.data.assign(candidates.begin(), candidates.end());

    level_count_ = 1;
    return OperationResult::kProduced;
}

WcojCore::OperationResult WcojCore::ExtendIntersect(Graph const& pattern) {
    intersection_cache_.last_isect_valid = false;

    assert(level_count_ > 0 && "Scan call is required before ExtendIntersect call");
    if (level_count_ >= qvo_.size()) {
        return OperationResult::kFinished;
    }

    auto const new_pv = qvo_[level_count_];
    std::size_t const parent_count = cur_level_.Count();

    next_level_.Reset(level_count_ + 1);
    // at least one child per parent
    next_level_.data.reserve(cur_level_.data.size() + parent_count);

    auto const& descriptors = BuildDescriptorsFor(pattern, new_pv, level_count_);
    for (std::size_t i = 0; i < parent_count; ++i) {
        std::span<Vertex const> const parent = cur_level_.Row(i);
        std::vector<Vertex> const& extension_set = ComputeExtensionSet(parent, new_pv, descriptors);

        for (Vertex const graph_vertex : extension_set) {
            // after the intersection cache lookup, so the cache is not affected
            if (semantics_ == MatchSemantics::kIsomorphism &&
                std::ranges::find(parent, graph_vertex) != parent.end()) {
                continue;
            }
            next_level_.PushRow(parent, graph_vertex);
        }
    }

    if (next_level_.Count() == 0) {
        return OperationResult::kEmpty;
    }

    std::swap(cur_level_, next_level_);
    ++level_count_;

    return level_count_ == qvo_.size() ? OperationResult::kFinished : OperationResult::kProduced;
}

std::vector<WcojCore::AdjacencyDescriptor> WcojCore::BuildDescriptorsFor(Graph const& pattern,
                                                                         Vertex new_pv,
                                                                         std::size_t level) const {
    assert(level <= qvo_.size() && "BuildDescriptorsFor level is greater than qvo_.size()");

    std::vector<AdjacencyDescriptor> descriptors;
    for (std::size_t i = 0; i < level; ++i) {
        Vertex const old_pv = qvo_[i];

        for (auto const edge : boost::make_iterator_range(boost::out_edges(old_pv, pattern))) {
            if (boost::target(edge, pattern) == new_pv) {
                descriptors.push_back(AdjacencyDescriptor{
                        .pattern_vertex = old_pv,
                        .qvo_index = i,
                        .direction = Direction::kOut,
                        .edge_label = (pattern)[edge].label,
                });
            }
        }

        for (auto const edge : boost::make_iterator_range(boost::in_edges(old_pv, pattern))) {
            if (boost::source(edge, pattern) == new_pv) {
                descriptors.push_back(AdjacencyDescriptor{
                        .pattern_vertex = old_pv,
                        .qvo_index = i,
                        .direction = Direction::kIn,
                        .edge_label = (pattern)[edge].label,
                });
            }
        }
    }

    return descriptors;
}

std::vector<WcojCore::Vertex> const& WcojCore::GetNeighbors(Vertex graph_vertex,
                                                            Direction direction,
                                                            std::string_view edge_label) {
    NeighborKeyLookup const lookup(graph_vertex, direction, edge_label);

    if (auto const it = adjacency_index_.find(lookup); it != adjacency_index_.end()) {
        return it->second;
    }

    NeighborKey key{.graph_vertex = lookup.graph_vertex,
                    .direction = lookup.direction,
                    .edge_label = std::string(lookup.edge_label_view)};
    auto [it, inserted] = adjacency_index_.emplace(std::move(key), std::vector<Vertex>{});
    std::vector<Vertex>& neighbors = it->second;

    if (direction == Direction::kOut) {
        for (auto const edge : boost::make_iterator_range(boost::out_edges(graph_vertex, graph_))) {
            if (model::gdd::LabelsMatch(edge_label, graph_[edge].label)) {
                neighbors.push_back(boost::target(edge, graph_));
            }
        }
    } else {
        for (auto const edge : boost::make_iterator_range(boost::in_edges(graph_vertex, graph_))) {
            if (model::gdd::LabelsMatch(edge_label, graph_[edge].label)) {
                neighbors.push_back(boost::source(edge, graph_));
            }
        }
    }

    std::ranges::sort(neighbors);
    auto const [first, last] = std::ranges::unique(neighbors.begin(), neighbors.end());
    neighbors.erase(first, last);
    return neighbors;
}

std::vector<WcojCore::Vertex> const& WcojCore::ComputeExtensionSet(
        std::span<Vertex const> partial_match, Vertex new_pv,
        std::vector<AdjacencyDescriptor> const& descriptors) {
    auto const domain_it = domain_.find(new_pv);
    if (domain_it == domain_.end() || domain_it->second.empty()) {
        intersection_cache_.last_isect_valid = false;
        intersection_cache_.last_isect_key.clear();
        intersection_cache_.last_isect_set.clear();
        return intersection_cache_.last_isect_set;
    }

    std::vector<Vertex> const& domain = domain_it->second;

    if (descriptors.empty()) {
        intersection_cache_.last_isect_valid = false;
        return domain;
    }

    auto& [cache_key, cache_set, cache_valid] = intersection_cache_;

    // view of mapped descriptors
    auto const mapped_descriptors =
            descriptors | std::views::transform([&partial_match](auto const& d) {
                return partial_match[d.qvo_index];
            });

    // lookup intersection cache
    // NB: materialize view only if sizes are the same
    if (cache_valid && descriptors.size() == cache_key.size() &&
        std::ranges::equal(cache_key, mapped_descriptors)) {
        return cache_set;
    }

    std::vector<std::vector<Vertex> const*> lists;
    lists.reserve(descriptors.size() + 1);
    lists.push_back(&domain);

    for (auto const& d : descriptors) {
        Vertex const mapped = partial_match[d.qvo_index];
        std::vector<Vertex> const& neighbors = GetNeighbors(mapped, d.direction, d.edge_label);

        if (neighbors.empty()) {
            cache_valid = true;
            cache_key.assign(mapped_descriptors.begin(), mapped_descriptors.end());
            cache_set.clear();
            return cache_set;
        }

        lists.push_back(&neighbors);
    }

    cache_key.assign(mapped_descriptors.begin(), mapped_descriptors.end());
    LeapFrogJoin(lists, cache_set);
    cache_valid = true;
    return cache_set;
}

void WcojCore::LeapFrogJoin(std::vector<std::vector<Vertex> const*>& lists,
                            std::vector<Vertex>& out) {
    out.clear();
    if (lists.empty()) {
        return;
    }

    if (lists.size() == 1) {
        out.assign(lists.front()->begin(), lists.front()->end());
        return;
    }

    std::ranges::sort(lists, [](auto const* a, auto const* b) { return a->size() < b->size(); });
    if (lists.front()->empty()) {
        return;
    }

    std::size_t const list_count = lists.size();
    cursors_.resize(list_count);
    for (std::size_t i = 0; i < list_count; ++i) {
        cursors_[i] = ListCursor{
                .cursor = lists[i]->data(),
                .end = lists[i]->data() + lists[i]->size(),
        };
    }

    // search for candidate in other lists
    Vertex candidate = *cursors_[0].cursor;
    // count of lists where candidate is matched
    std::size_t matched = 1;
    // current list index
    std::size_t index = 1;

    while (true) {
        auto& [cursor, list_end] = cursors_[index];
        cursor = LeapFrogSeek(cursor, list_end, candidate);
        if (cursor == list_end) {
            return;
        }

        if (*cursor == candidate) {
            if (++matched == list_count) {
                out.push_back(candidate);
                if (++cursor == list_end) {
                    return;
                }
                candidate = *cursor;
                matched = 1;
            }
        } else {
            candidate = *cursor;
            matched = 1;
        }

        index = index + 1 < list_count ? index + 1 : 0;
    }
}

}  // namespace algos::detail
