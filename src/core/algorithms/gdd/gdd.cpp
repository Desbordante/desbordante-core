#include "gdd.h"

#include <unordered_map>

#include <boost/graph/vf2_sub_graph_iso.hpp>

#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/util/levenshtein_distance.h"

namespace model {

namespace gdd::detail {

bool IsSubgraph(gdd::graph_t const& query, gdd::graph_t const& graph) {
    bool result = false;

    auto vcmp = [&query, &graph](gdd::vertex_t const& query_v, gdd::vertex_t const& graph_v) {
        return query[query_v].label == graph[graph_v].label &&  // TODO : handle wildcard
               query[query_v].attributes == graph[graph_v].attributes;
    };

    auto ecmp = [&query, &graph](gdd::edge_t const& query_e, gdd::edge_t const& graph_e) {
        return query[query_e].label == graph[graph_e].label;  // TODO: handle wildcard
    };

    auto callback = [&result](auto, auto) {
        result = true;
        return false;
    };

    using PropertyMapT = boost::property_map<gdd::graph_t, boost::vertex_index_t>::type;
    PropertyMapT const query_index_map = boost::get(boost::vertex_index, query);
    PropertyMapT const graph_index_map = boost::get(boost::vertex_index, graph);
    std::vector<gdd::vertex_t> const query_vertex_order = boost::vertex_order_by_mult(query);

    boost::vf2_subgraph_iso(query, graph, callback, query_index_map, graph_index_map,
                            query_vertex_order, ecmp, vcmp);
    return result;
}

}  // namespace gdd::detail

namespace {

bool CompareDistance(double dist, gdd::detail::CmpOp op, double threshold) {
    constexpr double eps = std::numeric_limits<double>::epsilon();
    using ::model::gdd::detail::CmpOp;

    switch (op) {
        case CmpOp::kLe:
            return dist <= threshold;
        case CmpOp::kGe:
            return dist >= threshold;
        case CmpOp::kLt:
            return dist < threshold;
        case CmpOp::kGt:
            return dist > threshold;
        case CmpOp::kEq:
            return std::abs(dist - threshold) <= eps;
        case CmpOp::kNe:
            return std::abs(dist - threshold) > eps;
        default:
            throw std::logic_error("Unimplemented distance compare operation");
    }
}

double TryParseNumber(gdd::detail::ConstValue const& val) {
    if (std::holds_alternative<int64_t>(val)) {
        return std::get<int64_t>(val);
    }
    if (std::holds_alternative<double>(val)) {
        return std::get<double>(val);
    }

    std::string const& s = std::get<std::string>(val);
    return std::stod(s);
}

double CalculateDistance(gdd::detail::ConstValue const& lhs, gdd::detail::ConstValue const& rhs,
                         gdd::detail::DistanceMetric metric_kind) {
    switch (metric_kind) {
        case gdd::detail::DistanceMetric::kAbsDiff:
            return std::abs(TryParseNumber(lhs) - TryParseNumber(rhs));

        case gdd::detail::DistanceMetric::kEditDistance:
            if (!std::holds_alternative<std::string>(lhs)) {
                throw std::logic_error("Expected string in LHS for edit distance metric");
            }
            if (!std::holds_alternative<std::string>(rhs)) {
                throw std::logic_error("Expected string in RHS for edit distance metric");
            }
            return util::LevenshteinDistance(std::get<std::string>(lhs),
                                             std::get<std::string>(rhs));

        default:
            throw std::logic_error("Unimplemented distance metric type");
    }
}

}  // namespace

std::optional<gdd::vertex_t> Gdd::FindPatternVertexById(size_t id) const {
    for (auto [v, vend] = boost::vertices(pattern_); v != vend; ++v) {
        if (pattern_[*v].id == id) return *v;
    }

    return std::nullopt;
}

bool Gdd::SatisfiesConstraint(gdd::graph_t const& g,
                              std::unordered_map<gdd::vertex_t, gdd::vertex_t> const& pg_map,
                              gdd::detail::DistanceConstraint const& constraint) const {
    using namespace gdd::detail;

    auto resolve_graph_vertex = [this, &pg_map](std::size_t pv_id) -> std::optional<gdd::vertex_t> {
        auto const pv = FindPatternVertexById(pv_id);
        if (!pv) return std::nullopt;

        auto const gv_it = pg_map.find(*pv);
        if (gv_it == pg_map.end()) {
            return std::nullopt;
        }

        return gv_it->second;
    };

    auto token_as_relation = [](DistanceOperand const& operand)
            -> std::optional<std::pair<std::size_t, std::string>> {
        if (!std::holds_alternative<GddToken>(operand)) {
            return std::nullopt;
        }

        auto const& [pattern_vertex_id, field] = std::get<GddToken>(operand);
        if (!std::holds_alternative<RelTag>(field)) {
            return std::nullopt;
        }
        return std::make_pair(pattern_vertex_id, std::get<RelTag>(field).name);
    };

    auto collect_relation_targets = [&g](gdd::vertex_t gv, std::string const& rel_label) {
        std::unordered_set<gdd::vertex_t> targets;

        for (auto [edge_it, eend] = boost::out_edges(gv, g); edge_it != eend; ++edge_it) {
            if (std::string const& lab = g[*edge_it].label; lab == rel_label) {  // TODO: wildcards
                targets.insert(boost::target(*edge_it, g));
            }
        }
        return targets;
    };

    // 1. relation constraints
    if (auto const lhs_rel = token_as_relation(constraint.lhs)) {
        auto const& [lhs_pid, lhs_relname] = *lhs_rel;
        auto const gv_lhs_opt = resolve_graph_vertex(lhs_pid);

        if (!gv_lhs_opt) {
            return false;
        }
        auto const gv_lhs = *gv_lhs_opt;

        std::unordered_set<gdd::vertex_t> const lhs_targets =
                collect_relation_targets(gv_lhs, lhs_relname);

        // 1.1. exists edge with label lhs_relname that ends at node constraint.rhs
        if (std::holds_alternative<ConstValue>(constraint.rhs)) {
            ConstValue const& cv = std::get<ConstValue>(constraint.rhs);
            auto matches_cr = [&cv, &g](gdd::vertex_t t) {
                if (std::holds_alternative<int64_t>(cv)) {
                    return g[t].id == static_cast<size_t>(std::get<int64_t>(cv));
                }
                throw std::logic_error(
                        "Invalid node representation in gdd relation constraint (unsuitable type)");
            };
            return std::ranges::any_of(lhs_targets, matches_cr);
        }

        // 1.2. both nodes have edge with same label that ends at the same node
        if (auto const rhs_rel = token_as_relation(constraint.rhs); rhs_rel) {
            auto const& [rhs_pid, rhs_relname] = *rhs_rel;
            // TODO: wildcard
            if (lhs_relname != rhs_relname) {
                return false;
            }

            auto const gv_rhs_opt = resolve_graph_vertex(rhs_pid);
            if (!gv_rhs_opt) {
                return false;
            }
            auto const gv_rhs = *gv_rhs_opt;
            std::unordered_set<gdd::vertex_t> const rhs_targets =
                    collect_relation_targets(gv_rhs, rhs_relname);

            bool intersect = false;
            for (auto const& vertex : lhs_targets) {
                if (rhs_targets.contains(vertex)) {
                    intersect = true;
                    break;
                }
            }

            return intersect;
        }

        return false;
    }

    // 2. general attribute distance constraint
    auto resolve_scalar =
            [&g, &resolve_graph_vertex](DistanceOperand const& op) -> std::optional<ConstValue> {
        if (std::holds_alternative<ConstValue>(op)) {
            return std::get<ConstValue>(op);
        }

        auto const& [pattern_vertex_id, field] = std::get<GddToken>(op);
        auto const gv_opt = resolve_graph_vertex(pattern_vertex_id);
        if (!gv_opt) {
            return std::nullopt;
        }
        auto const gv = *gv_opt;

        std::string const& name = std::get<AttrTag>(field).name;
        if (name == "id") {
            return static_cast<int64_t>(g[gv].id);
        }
        if (name == "label") {
            return g[gv].label;
        }

        if (auto const it = g[gv].attributes.find(name); it == g[gv].attributes.end()) {
            return std::nullopt;
        } else {
            return it->second;
        }
    };

    auto const lhs_scalar = resolve_scalar(constraint.lhs);
    auto const rhs_scalar = resolve_scalar(constraint.rhs);

    if (!lhs_scalar || !rhs_scalar) {
        return false;
    }

    double const dist = CalculateDistance(*lhs_scalar, *rhs_scalar, constraint.metric);

    return CompareDistance(dist, constraint.op, constraint.threshold);
}

GddCounterexample BuildCounterexample(
        gdd::graph_t const& pattern, gdd::graph_t const& graph,
        std::unordered_map<gdd::vertex_t, gdd::vertex_t> const& mapping) {
    GddCounterexample ce{};
    ce.match.reserve(mapping.size());

    for (auto const& [pv, gv] : mapping) {
        ce.match.push_back({
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
}  // namespace model