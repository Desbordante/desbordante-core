#include "naive_gdd_validator.h"

namespace algos {

bool NaiveGddValidator::Holds(model::Gdd const& gdd, model::gdd::graph_t const& graph,
                              GddCounterexample& out_counterexample) {
    model::gdd::graph_t const& pattern = gdd.GetPattern();
    if (domain_ = BuildDomain(pattern, graph); domain_.size() == boost::num_vertices(pattern)) {
        MappingT partial_map;
        bool const exists_counterexample =
                ExistsCounterexample(gdd, graph, partial_map, out_counterexample);
        return !exists_counterexample;
    }

    return true;
}

NaiveGddValidator::DomainT NaiveGddValidator::BuildDomain(model::gdd::graph_t const& pattern,
                                                          model::gdd::graph_t const& graph) {
    std::unordered_map<model::gdd::vertex_t, std::vector<model::gdd::vertex_t>> dom;

    boost::graph_traits<model::gdd::graph_t>::vertex_iterator pv;
    boost::graph_traits<model::gdd::graph_t>::vertex_iterator pend;

    for (std::tie(pv, pend) = boost::vertices(pattern); pv != pend; ++pv) {
        boost::graph_traits<model::gdd::graph_t>::vertex_iterator gv;
        boost::graph_traits<model::gdd::graph_t>::vertex_iterator gend;

        for (std::tie(gv, gend) = boost::vertices(graph); gv != gend; ++gv) {
            if (pattern[*pv].label == graph[*gv].label) {  // TODO: wildcards
                dom[*pv].emplace_back(*gv);
            }
        }
    }

    return dom;
}

GddCounterexample NaiveGddValidator::BuildCounterexample(model::gdd::graph_t const& pattern,
                                                         model::gdd::graph_t const& graph,
                                                         MappingT const& mapping) {
    GddCounterexample ce{};
    ce.match.reserve(mapping.size());

    for (auto const& [pv, gv] : mapping) {
        ce.match.push_back(GddCounterexampleVertex{
                .pattern_vertex_id = pattern[pv].id,
                .pattern_vertex_label = pattern[pv].label,
                .graph_vertex_id = graph[gv].id,
                .graph_vertex_label = graph[gv].label,
                .graph_vertex_attributes = graph[gv].attributes,
        });
    }

    std::ranges::sort(ce.match, {}, &GddCounterexampleVertex::pattern_vertex_id);
    return ce;
}

bool NaiveGddValidator::ExistsCounterexample(model::Gdd const& gdd,
                                             model::gdd::graph_t const& graph,
                                             MappingT& partial_map,
                                             GddCounterexample& out_counterexample) {
    // Naming of local variables is messy.
    // z - pattern variable from domain (like in paper)
    // gv/pv - graph/pattern vertex
    // ge/pe - graph/pattern edge

    if (partial_map.size() == domain_.size()) {
        bool const sat = gdd.Satisfies(graph, partial_map);

        if (!sat) {
            out_counterexample = BuildCounterexample(gdd.GetPattern(), graph, partial_map);
        }
        return !sat;
    }

    auto const& pattern = gdd.GetPattern();

    auto are_adjacent_in_pattern = [&pattern](VertexT lhs, VertexT rhs) {
        return boost::edge(lhs, rhs, pattern).second || boost::edge(rhs, lhs, pattern).second;
    };

    // does pattern edge preserve in graph?
    auto has_compatible_edge = [&pattern, &graph](VertexT pv, VertexT mapped_pv, VertexT z,
                                                  VertexT gv) {
        auto const matches_edge = [&pattern, &graph](VertexT graph_src, VertexT graph_dst,
                                                     VertexT pattern_src, VertexT pattern_dst) {
            auto const [ge, ge_exists] = boost::edge(graph_src, graph_dst, graph);
            if (!ge_exists) {
                return false;
            }

            auto const [pe, pe_exists] = boost::edge(pattern_src, pattern_dst, pattern);
            return pe_exists && graph[ge].label == pattern[pe].label;  // TODO: wildcards
        };

        return matches_edge(mapped_pv, gv, pv, z) || matches_edge(gv, mapped_pv, z, pv);
    };

    // can map z to gv?
    auto can_extend_mapping = [&](VertexT z, VertexT gv) {
        if (partial_map.empty()) {
            return true;
        }

        bool const has_mapped_adjacent =
                std::ranges::any_of(partial_map, [&](auto const& mapped_pair) {
                    return are_adjacent_in_pattern(mapped_pair.first, z);
                });

        if (!has_mapped_adjacent) {
            return false;
        }

        return std::ranges::all_of(partial_map, [&](auto const& mapped_pair) {
            auto const& [pv, mapped_pv] = mapped_pair;
            return !are_adjacent_in_pattern(pv, z) || has_compatible_edge(pv, mapped_pv, z, gv);
        });
    };

    for (auto const& [z, gv_candidates] : domain_) {
        if (partial_map.contains(z)) {
            continue;
        }

        for (VertexT gv : gv_candidates) {
            if (!can_extend_mapping(z, gv)) {
                continue;
            }

            auto const [it, inserted] = partial_map.emplace(z, gv);
            if (!inserted) {
                continue;
            }

            if (ExistsCounterexample(gdd, graph, partial_map, out_counterexample)) {
                return true;
            }

            partial_map.erase(it);
        }
    }

    return false;
}

}  // namespace algos