#include "naive_gdd_validator.h"

namespace algos {

GddValidator::GddHoldsResult NaiveGddValidator::Holds(model::Gdd const& gdd,
                                                      model::gdd::graph_t const& graph) {
    match_count_ = 0;
    model::gdd::graph_t const& pattern = gdd.GetPattern();
    if (domain_ = BuildDomain(pattern, graph); domain_.size() == boost::num_vertices(pattern)) {
        MappingT partial_map;
        partial_map.reserve(boost::num_vertices(pattern));
        if (GddCounterexample counterexample{};
            ExistsCounterexample(gdd, graph, partial_map, counterexample)) {
            return {counterexample, match_count_};
        }
    }

    return {std::nullopt, match_count_};
}

std::unique_ptr<GddValidator> NaiveGddValidator::CreateWorker() const {
    return std::make_unique<NaiveGddValidator>();
}

bool NaiveGddValidator::GraphHasCompatibleEdge(model::gdd::graph_t const& graph, VertexT graph_src,
                                               VertexT graph_dst,
                                               std::string const& pattern_edge_label) {
    for (auto const graph_edge : boost::make_iterator_range(boost::out_edges(graph_src, graph))) {
        if (boost::target(graph_edge, graph) != graph_dst) {
            continue;
        }

        if (model::Gdd::LabelsMatch(pattern_edge_label, graph[graph_edge].label)) {
            return true;
        }
    }

    return false;
}

bool NaiveGddValidator::AllPatternEdgesArePreserved(model::gdd::graph_t const& graph,
                                                    model::gdd::graph_t const& pattern,
                                                    VertexT pattern_src, VertexT pattern_dst,
                                                    VertexT graph_src, VertexT graph_dst) {
    for (auto const pattern_edge :
         boost::make_iterator_range(boost::out_edges(pattern_src, pattern))) {
        if (boost::target(pattern_edge, pattern) != pattern_dst) {
            continue;
        }

        if (!GraphHasCompatibleEdge(graph, graph_src, graph_dst, pattern[pattern_edge].label)) {
            return false;
        }
    }

    return true;
}

bool NaiveGddValidator::CanExtendMapping(model::gdd::graph_t const& graph,
                                         MappingT const& partial_map,
                                         model::gdd::graph_t const& pattern, VertexT pattern_var,
                                         VertexT graph_vertex) {
    return std::ranges::all_of(partial_map, [&](auto const& mapped_pair) {
        auto const& [mapped_pattern_var, mapped_graph_vertex] = mapped_pair;

        return AllPatternEdgesArePreserved(graph, pattern, mapped_pattern_var, pattern_var,
                                           mapped_graph_vertex, graph_vertex) &&
               AllPatternEdgesArePreserved(graph, pattern, pattern_var, mapped_pattern_var,
                                           graph_vertex, mapped_graph_vertex);
    });
}

bool NaiveGddValidator::ExistsCounterexample(model::Gdd const& gdd,
                                             model::gdd::graph_t const& graph,
                                             MappingT& partial_map,
                                             GddCounterexample& counterexample) {
    if (partial_map.size() == domain_.size()) {
        ++match_count_;
        bool const sat = gdd.Satisfies(graph, partial_map);

        if (!sat) {
            counterexample = model::BuildCounterexample(gdd.GetPattern(), graph, partial_map);
        }
        return !sat;
    }

    auto const& pattern = gdd.GetPattern();
    for (auto const& [pattern_var, graph_vertex_candidates] : domain_) {
        if (partial_map.contains(pattern_var)) {
            continue;
        }

        for (VertexT graph_vertex : graph_vertex_candidates) {
            if (!CanExtendMapping(graph, partial_map, pattern, pattern_var, graph_vertex)) {
                continue;
            }

            auto const [_, inserted] = partial_map.emplace(pattern_var, graph_vertex);
            if (!inserted) {
                continue;
            }

            if (ExistsCounterexample(gdd, graph, partial_map, counterexample)) {
                return true;
            }

            partial_map.erase(pattern_var);
        }
        break;  // other pattern variables are assigned on the next recursion levels
    }

    return false;
}

}  // namespace algos
