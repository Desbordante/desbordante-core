#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>

#include <boost/range/iterator_range.hpp>

#include "core/algorithms/gdd/gdd_graph_description.h"

namespace algos {

template <typename F>
concept MatchCallback = std::predicate<F, model::gdd::GraphTypes::Mapping const&>;

enum class MatchSemantics : std::uint8_t { kHomomorphism, kIsomorphism };

// this graph matcher works with gdd property graphs (directed, with labels on each vertex and edge)
// callback receives a match (pattern vertex -> graph vertex), returns false to stop the search,
// true to continue searching
// note: virtual inheritance for GraphTypes because child classes can't see protected/public names
// of base class directly (base depends on template argument), so they also inherit GraphTypes
// note: for the same reason protected and public methods and fields in child classes are accessed
// explicitly, through `this` pointer
template <MatchCallback Callback>
class GraphMatcher : private virtual model::gdd::GraphTypes {
public:
    template <typename F>
        requires std::constructible_from<Callback, F>
    GraphMatcher(Graph const& graph, F&& callback)
        : graph_(graph), callback_(std::forward<F>(callback)) {}

    template <typename F>
    GraphMatcher(Graph&&, F&&) = delete;

    virtual ~GraphMatcher() = default;

    // resets caches if needed. if graph is externally changed since last `Match` call,
    // reset is required (caches are in the invalid state)
    virtual void Reset() {}

    // enumerates matches of pattern in the graph and calls OnMatch on every match
    // returns false if the search was stopped by the callback
    bool Run(Graph const& pattern, MatchSemantics semantics = MatchSemantics::kHomomorphism) {
        assert(!is_matching_ && "Match inner call from callback is prohibited");
        is_matching_ = true;
        bool not_stopped = false;
        try {
            not_stopped = DoMatch(pattern, BuildDomain(pattern, graph_), semantics);
            is_matching_ = false;
        } catch (...) {
            is_matching_ = false;
            throw;
        }
        return not_stopped;
    }

protected:
    bool OnMatch(Mapping const& mapping) {
        return std::invoke(callback_, mapping);
    }

    Graph const& graph_;

private:
    Callback callback_;
    bool is_matching_ = false;

    virtual bool DoMatch(Graph const& pattern, Domain domain, MatchSemantics semantics) = 0;

    static bool HasLoop(Graph const& graph, Vertex v, std::string_view label) {
        for (auto const e : boost::make_iterator_range(boost::out_edges(v, graph))) {
            if (boost::target(e, graph) == v && model::gdd::LabelsMatch(label, graph[e].label)) {
                return true;
            }
        }
        return false;
    }

    static bool LoopsPreserved(Graph const& pattern, Vertex pv, Graph const& graph, Vertex gv) {
        for (auto const e : boost::make_iterator_range(boost::out_edges(pv, pattern))) {
            if (boost::target(e, pattern) == pv && !HasLoop(graph, gv, pattern[e].label)) {
                return false;
            }
        }
        return true;
    }

    static Domain BuildDomain(Graph const& pattern, Graph const& graph) {
        Domain dom;

        for (auto const pv : boost::make_iterator_range(boost::vertices(pattern))) {
            for (auto const gv : boost::make_iterator_range(boost::vertices(graph))) {
                if (model::gdd::LabelsMatch(pattern[pv].label, graph[gv].label) &&
                    LoopsPreserved(pattern, pv, graph, gv)) {
                    dom[pv].emplace_back(gv);
                }
            }
        }

        return dom;
    }
};

}  // namespace algos
