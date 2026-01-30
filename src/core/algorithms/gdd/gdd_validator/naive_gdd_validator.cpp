#include "naive_gdd_validator.h"

#include "spdlog/spdlog.h"

namespace algos {

bool NaiveGddValidator::Holds(model::Gdd const& gdd, model::gdd::graph_t const& graph) {
    model::gdd::graph_t const& pattern = gdd.GetPattern();
    if (domain_ = BuildDomain(pattern, graph); domain_.size() == boost::num_vertices(pattern)) {
        MappingT partial_map;
        return !ExistsCounterexample(gdd, graph, partial_map);
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

bool NaiveGddValidator::ExistsCounterexample(model::Gdd const& gdd,
                                             model::gdd::graph_t const& graph,
                                             MappingT& partial_map) {
    if (partial_map.size() == domain_.size()) {
        if (!gdd.Satisfies(graph, partial_map)) {
            return true;
        }
        return false;
    }

    auto const& pattern = gdd.GetPattern();

    // if there is edge (pv, z) and edge (mapped_pv, gv) which are compatible by label
    // (in any direction)
    auto can_connect_zgv = [&pattern, &graph](vertex_t const pv, vertex_t const mapped_pv,
                                              vertex_t const z, vertex_t const gv) {
        for (auto const& [u, v, x, y] :
             {std::tuple{mapped_pv, gv, pv, z}, std::tuple{gv, mapped_pv, z, pv}}) {
            if (auto const& [ge, ge_exists] = boost::edge(u, v, graph); ge_exists) {
                if (auto const& [pe, pe_exists] = boost::edge(x, y, pattern);
                    pe_exists && graph[ge].label == pattern[pe].label) {  // TODO: wildcards
                    return true;
                }
            }
        }
        return false;
    };

    for (auto const& [z, gv_candidates] : domain_) {
        if (partial_map.contains(z)) {
            continue;
        }
        for (auto const gv : gv_candidates) {
            bool const can_assign_gv =
                    partial_map.empty() ||
                    std::ranges::any_of(partial_map, [z, gv, &can_connect_zgv](auto const t) {
                        return can_connect_zgv(t.first, t.second, z, gv);
                    });

            if (can_assign_gv) {
                auto const [it, inserted] = partial_map.emplace(z, gv);
                if (!inserted) {
                    continue;
                }
                if (ExistsCounterexample(gdd, graph, partial_map)) {
                    return true;
                }
                partial_map.erase(it);
            }
        }
    }
    return false;
}

}  // namespace algos