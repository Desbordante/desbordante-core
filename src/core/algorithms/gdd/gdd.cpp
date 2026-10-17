#include "gdd.h"

#include <unordered_map>

#include <boost/graph/vf2_sub_graph_iso.hpp>

#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/util/levenshtein_distance.h"
#include "graph_matcher/wcoj_graph_matcher.h"

namespace model {

namespace gdd::detail {

bool IsSubgraph(GraphTypes::Graph const& query, GraphTypes::Graph const& graph) {
    algos::WcojGraphMatcher matcher(graph, [](auto) { return false; });
    return !matcher.Run(query, algos::MatchSemantics::kIsomorphism);
}

}  // namespace gdd::detail

namespace {

bool CompareDistance(double dist, gdd::detail::CmpOp op, double threshold) {
    constexpr double eps = std::numeric_limits<double>::epsilon();
    using gdd::detail::CmpOp;

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
    if (std::holds_alternative<std::int64_t>(val)) {
        return std::get<std::int64_t>(val);
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

std::size_t Gdd::ExtractVertexIdFromConst(gdd::detail::ConstValue const& cv) {
    if (std::holds_alternative<std::int64_t>(cv)) {
        if (auto const id = std::get<std::int64_t>(cv); id >= 0) {
            return static_cast<std::size_t>(id);
        }
        throw std::out_of_range("Invalid vertex id (negative number)");
    }
    throw std::logic_error("Invalid vertex id (unsuitable type)");
}

std::optional<Gdd::Vertex> Gdd::FindPatternVertexById(std::size_t id) const {
    auto const& verts = boost::make_iterator_range(boost::vertices(pattern_));
    auto const it =
            std::ranges::find_if(verts, [this, id](Vertex v) { return pattern_[v].id == id; });
    return it == verts.end() ? std::nullopt : std::optional{*it};
}

std::optional<Gdd::Vertex> Gdd::ResolveGraphVertex(Mapping const& pg_map, std::size_t pv_id) const {
    if (auto const pv = FindPatternVertexById(pv_id); pv) {
        auto const gv_it = pg_map.find(*pv);
        if (gv_it == pg_map.end()) {
            return std::nullopt;
        }
        return gv_it->second;
    }

    return std::nullopt;
}

std::optional<std::pair<std::size_t, std::string_view>> Gdd::TokenAsRelation(
        gdd::detail::DistanceOperand const& operand) {
    using namespace gdd::detail;

    if (!std::holds_alternative<GddToken>(operand)) {
        return std::nullopt;
    }

    auto const& [pattern_vertex_id, field] = std::get<GddToken>(operand);
    if (!std::holds_alternative<RelTag>(field)) {
        return std::nullopt;
    }
    return std::pair<std::size_t, std::string_view>{pattern_vertex_id,
                                                    std::get<RelTag>(field).name};
}

std::unordered_set<Gdd::Vertex> Gdd::CollectRelationTargets(Graph const& g, Vertex gv,
                                                            std::string_view rel_label) {
    std::unordered_set<Vertex> targets;

    for (auto [edge_it, eend] = boost::out_edges(gv, g); edge_it != eend; ++edge_it) {
        if (std::string const& lab = g[*edge_it].label; gdd::LabelsMatch(lab, rel_label)) {
            targets.insert(boost::target(*edge_it, g));
        }
    }
    return targets;
}

std::optional<gdd::detail::ConstValue> Gdd::ResolveScalar(
        Graph const& g, Mapping const& pg_map, gdd::detail::DistanceOperand const& op) const {
    using namespace gdd::detail;

    if (std::holds_alternative<ConstValue>(op)) {
        return std::get<ConstValue>(op);
    }

    auto const& [pattern_vertex_id, field] = std::get<GddToken>(op);
    auto const gv_opt = ResolveGraphVertex(pg_map, pattern_vertex_id);
    if (!gv_opt) {
        return std::nullopt;
    }
    auto const gv = *gv_opt;

    std::string const& name = std::get<AttrTag>(field).name;
    if (name == "id") {
        if (g[gv].id > std::numeric_limits<std::int64_t>::max()) {
            throw std::out_of_range("Vertex id is too big to be resolved");
        }
        return static_cast<std::int64_t>(g[gv].id);
    }
    if (name == "label") {
        return g[gv].label;
    }

    auto const it = g[gv].attributes.find(name);
    if (it == g[gv].attributes.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool Gdd::SatisfiesRelationConstraint(Graph const& g, Mapping const& pg_map,
                                      std::pair<std::size_t, std::string_view> const& lhs_rel,
                                      gdd::detail::DistanceOperand const& rhs) const {
    using namespace gdd::detail;

    auto const& [lhs_pid, lhs_relname] = lhs_rel;
    auto const gv_lhs_opt = ResolveGraphVertex(pg_map, lhs_pid);
    if (!gv_lhs_opt) {
        return false;
    }

    std::unordered_set<Vertex> const lhs_targets =
            CollectRelationTargets(g, *gv_lhs_opt, lhs_relname);

    // 1. exists edge with label lhs_relname that ends at node rhs
    if (std::holds_alternative<ConstValue>(rhs)) {
        ConstValue const& cv = std::get<ConstValue>(rhs);
        return std::ranges::any_of(lhs_targets, [&g, &cv](Vertex target) {
            return g[target].id == ExtractVertexIdFromConst(cv);
        });
    }

    // 2. both nodes have edge with same label that ends at the same node
    if (auto const rhs_rel = TokenAsRelation(rhs)) {
        auto const& [rhs_pid, rhs_relname] = *rhs_rel;
        if (!gdd::LabelsMatch(lhs_relname, rhs_relname)) {
            return false;
        }

        auto const gv_rhs_opt = ResolveGraphVertex(pg_map, rhs_pid);
        if (!gv_rhs_opt) {
            return false;
        }
        std::unordered_set<Vertex> const rhs_targets =
                CollectRelationTargets(g, *gv_rhs_opt, rhs_relname);

        return std::ranges::any_of(lhs_targets,
                                   [&rhs_targets](Vertex v) { return rhs_targets.contains(v); });
    }

    return false;
}

bool Gdd::SatisfiesAttributeConstraint(Graph const& g, Mapping const& pg_map,
                                       gdd::detail::DistanceConstraint const& constraint) const {
    auto const lhs_scalar = ResolveScalar(g, pg_map, constraint.lhs);
    auto const rhs_scalar = ResolveScalar(g, pg_map, constraint.rhs);

    if (!lhs_scalar || !rhs_scalar) {
        return false;
    }

    double const dist = CalculateDistance(*lhs_scalar, *rhs_scalar, constraint.metric);
    return CompareDistance(dist, constraint.op, constraint.threshold);
}

bool Gdd::SatisfiesConstraint(Graph const& g, Mapping const& pg_map,
                              gdd::detail::DistanceConstraint const& constraint) const {
    if (auto const lhs_rel = TokenAsRelation(constraint.lhs)) {
        return SatisfiesRelationConstraint(g, pg_map, *lhs_rel, constraint.rhs);
    }
    return SatisfiesAttributeConstraint(g, pg_map, constraint);
}

GddCounterexample BuildCounterexample(gdd::GraphTypes::Graph const& pattern,
                                      gdd::GraphTypes::Graph const& graph,
                                      gdd::GraphTypes::Mapping const& mapping) {
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
