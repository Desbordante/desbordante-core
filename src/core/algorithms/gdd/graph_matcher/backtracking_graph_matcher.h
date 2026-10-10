#pragma once

#include <string>
#include <utility>

#include "graph_matcher.h"

namespace algos {

namespace detail {

// independent from callback type, separated from BacktrackingGraphMatcher for compilation time
// purposes (we can place definitions in .cpp file)
class BacktrackingCore : private model::gdd::GraphTypes {
public:
    static bool GraphHasCompatibleEdge(Graph const& graph, Vertex graph_src, Vertex graph_dst,
                                       std::string const& pattern_edge_label);
    static bool AllPatternEdgesArePreserved(Graph const& graph, Graph const& pattern,
                                            Vertex pattern_src, Vertex pattern_dst,
                                            Vertex graph_src, Vertex graph_dst);
    static bool CanExtendMapping(Graph const& graph, Mapping const& partial_map,
                                 Graph const& pattern, Vertex pattern_var, Vertex graph_vertex,
                                 bool injective);
};

}  // namespace detail

template <MatchCallback Callback>
class BacktrackingGraphMatcher final : public GraphMatcher<Callback>,
                                       private virtual model::gdd::GraphTypes {
public:
    using GraphMatcher<Callback>::GraphMatcher;

private:
    Domain domain_;

    virtual bool DoMatch(Graph const& pattern, Domain domain, bool injective) final {
        if (domain_ = std::move(domain); domain_.size() != boost::num_vertices(pattern)) {
            return true;
        }

        Mapping partial_map;
        partial_map.reserve(boost::num_vertices(pattern));
        return Extend(pattern, partial_map, injective);
    }

    // returns false if the search was stopped by the callback
    bool Extend(Graph const& pattern, Mapping& partial_map, bool injective) {
        if (partial_map.size() == domain_.size()) {
            return this->OnMatch(partial_map);
        }

        for (auto const& [pattern_var, graph_vertex_candidates] : domain_) {
            if (partial_map.contains(pattern_var)) {
                continue;
            }

            for (Vertex graph_vertex : graph_vertex_candidates) {
                if (!detail::BacktrackingCore::CanExtendMapping(this->graph_, partial_map, pattern,
                                                                pattern_var, graph_vertex,
                                                                injective)) {
                    continue;
                }

                auto const [_, inserted] = partial_map.emplace(pattern_var, graph_vertex);
                if (!inserted) {
                    continue;
                }

                if (!Extend(pattern, partial_map, injective)) {
                    return false;
                }

                partial_map.erase(pattern_var);
            }
            break;  // other pattern variables are assigned on the next recursion levels
        }

        return true;
    }
};

}  // namespace algos
