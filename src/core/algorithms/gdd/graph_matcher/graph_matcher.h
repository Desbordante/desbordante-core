#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <utility>

#include <boost/range/iterator_range.hpp>

#include "core/algorithms/gdd/gdd_graph_description.h"

namespace algos {

template <typename F>
concept MatchCallback = std::predicate<F, model::gdd::GraphTypes::Mapping const&>;

// this graph matcher works with gdd property graphs (directed, with labels on each vertex and edge)
// callback receives a match (pattern vertex -> graph vertex) and returns false to stop the search
// note: virtual inheritance for GraphTypes because child classes can't see protected/public names
// of base class directly (base depends on template argument), so they also inherit GraphTypes; for
// the same reason protected and public methods and fields in child classes are accessed explicitly,
// through `this` pointer
template <MatchCallback Callback>
class GraphMatcher : private virtual model::gdd::GraphTypes {
public:
    enum class Semantics : std::uint8_t { kHomomorphism, kIsomorphism };

    template <typename F>
        requires std::constructible_from<Callback, F>
    GraphMatcher(Graph const& graph, F&& callback)
        : graph_(graph), callback_(std::forward<F>(callback)) {}

    virtual ~GraphMatcher() = default;

    // resets caches if needed
    virtual void Reset() {}

    // enumerates matches of pattern in the graph and calls OnMatch on every match
    // returns false if the search was stopped by the callback
    bool Match(Graph const& pattern, Semantics semantics = Semantics::kHomomorphism) {
        return DoMatch(pattern, BuildDomain(pattern, graph_), semantics == Semantics::kIsomorphism);
    }

protected:
    bool OnMatch(Mapping const& mapping) {
        return std::invoke(callback_, mapping);
    }

    Graph const& graph_;

private:
    Callback callback_;

    virtual bool DoMatch(Graph const& pattern, Domain domain, bool injective) = 0;

    static Domain BuildDomain(Graph const& pattern, Graph const& graph) {
        Domain dom;

        for (auto const pv : boost::make_iterator_range(boost::vertices(pattern))) {
            for (auto const gv : boost::make_iterator_range(boost::vertices(graph))) {
                if (model::gdd::LabelsMatch(pattern[pv].label, graph[gv].label)) {
                    dom[pv].emplace_back(gv);
                }
            }
        }

        return dom;
    }
};

}  // namespace algos
