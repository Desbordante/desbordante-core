#include "naive_gdd_validator.h"

#include "core/util/logger.h"
#include "spdlog/spdlog.h"

namespace algos {

namespace {

std::string FormatVertex(model::gdd::graph_t const& graph, model::gdd::vertex_t v) {
    std::ostringstream out;
    out << "{id=" << graph[v].id << ", label=\"" << graph[v].label << "\"";

    if (!graph[v].attributes.empty()) {
        out << ", attrs={";
        bool first = true;
        for (auto const& [key, value] : graph[v].attributes) {
            if (!first) {
                out << ", ";
            }
            first = false;
            out << key << "=\"" << value << '"';
        }
        out << "}";
    }

    out << "}";
    return out.str();
}

std::string FormatMapping(
        model::gdd::graph_t const& pattern, model::gdd::graph_t const& graph,
        std::unordered_map<model::gdd::vertex_t, model::gdd::vertex_t> const& map) {
    std::vector<std::pair<std::size_t, std::string>> rows;
    rows.reserve(map.size());

    for (auto const& [pv, gv] : map) {
        std::ostringstream out;
        out << "pattern[id=" << pattern[pv].id << ", label=\"" << pattern[pv].label << "\"]"
            << " -> " << FormatVertex(graph, gv);
        rows.emplace_back(pattern[pv].id, out.str());
    }

    std::ranges::sort(rows, {}, &std::pair<std::size_t, std::string>::first);

    std::ostringstream out;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (i != 0) {
            out << "; ";
        }
        out << rows[i].second;
    }
    return out.str();
}

}  // namespace

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
                                             MappingT& partial_map, std::size_t depth) {
    // Naming of local variables is messy.
    // z - pattern variable from domain (like in paper)
    // gv/pv - graph/pattern vertex
    // ge/pe - graph/pattern edge

    if (partial_map.size() == domain_.size()) {
        bool const sat = gdd.Satisfies(graph, partial_map);

        if (!sat && GetPrintReasonFlag()) {
            LOG_ERROR("Found GDD counterexample: lhs_size={}, rhs_size={}, match={}",
                      gdd.GetLhs().size(), gdd.GetRhs().size(),
                      FormatMapping(gdd.GetPattern(), graph, partial_map));
        }
        return !sat;
    }

    auto const& pattern = gdd.GetPattern();

    auto are_adjacent_in_pattern = [&pattern](vertex_t lhs, vertex_t rhs) {
        return boost::edge(lhs, rhs, pattern).second || boost::edge(rhs, lhs, pattern).second;
    };

    // does pattern edge preserve in graph?
    auto has_compatible_edge = [&pattern, &graph](vertex_t pv, vertex_t mapped_pv, vertex_t z,
                                                  vertex_t gv) {
        auto const matches_edge = [&pattern, &graph](vertex_t graph_src, vertex_t graph_dst,
                                                     vertex_t pattern_src, vertex_t pattern_dst) {
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
    auto can_extend_mapping = [&](vertex_t z, vertex_t gv) {
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

        for (vertex_t gv : gv_candidates) {
            if (!can_extend_mapping(z, gv)) {
                continue;
            }

            auto const [it, inserted] = partial_map.emplace(z, gv);
            if (!inserted) {
                continue;
            }

            if (ExistsCounterexample(gdd, graph, partial_map, ++depth)) {
                return true;
            }

            partial_map.erase(it);
        }
    }

    return false;
}

}  // namespace algos