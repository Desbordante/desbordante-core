#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>

#include <boost/container/flat_map.hpp>

#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/util/export.h"

namespace model {

namespace gdd::detail {

struct AttrTag {
    std::string name;

    bool operator==(AttrTag const&) const = default;
};

struct RelTag {
    std::string name;

    bool operator==(RelTag const&) const = default;
};

// entity id from paper is currently out of scope
using TokenField = std::variant<AttrTag, RelTag>;

struct GddToken {
    std::size_t pattern_vertex_id;
    TokenField field;

    bool operator==(GddToken const&) const = default;
};

using ConstValue = std::variant<std::int64_t, double, std::string>;
using DistanceOperand = std::variant<GddToken, ConstValue>;

enum class DistanceMetric : std::uint8_t { kAbsDiff, kEditDistance };
enum class CmpOp : std::uint8_t { kLe, kGe, kLt, kGt, kEq, kNe };

struct DistanceConstraint {
    DistanceOperand lhs;
    DistanceOperand rhs;
    double threshold;
    DistanceMetric metric;
    CmpOp op;

    bool operator==(DistanceConstraint const& other) const {
        return lhs == other.lhs && rhs == other.rhs &&
               std::abs(threshold - other.threshold) <= std::numeric_limits<double>::epsilon() &&
               metric == other.metric && op == other.op;
    }
};

bool IsSubgraph(gdd::graph_t const& query, gdd::graph_t const& graph);

using VertexT = model::gdd::vertex_t;
using EdgeT = model::gdd::edge_t;
using DomainT = std::unordered_map<VertexT, std::vector<VertexT>>;
using MappingT = boost::container::flat_map<VertexT, VertexT>;

}  // namespace gdd::detail

class DESBORDANTE_EXPORT Gdd {
public:
    using Phi = std::vector<gdd::detail::DistanceConstraint>;

private:
    gdd::graph_t pattern_;
    Phi lhs_;
    Phi rhs_;

    static std::size_t ExtractVertexIdFromConst(gdd::detail::ConstValue const& cv);

    static std::optional<std::pair<std::size_t, std::string>> TokenAsRelation(
            gdd::detail::DistanceOperand const& operand);

    static std::unordered_set<gdd::vertex_t> CollectRelationTargets(gdd::graph_t const& g,
                                                                    gdd::vertex_t gv,
                                                                    std::string const& rel_label);

    std::optional<gdd::vertex_t> FindPatternVertexById(std::size_t id) const;

    std::optional<gdd::vertex_t> ResolveGraphVertex(gdd::detail::MappingT const& pg_map,
                                                    std::size_t pv_id) const;

    std::optional<gdd::detail::ConstValue> ResolveScalar(
            gdd::graph_t const& g, gdd::detail::MappingT const& pg_map,
            gdd::detail::DistanceOperand const& op) const;

    bool SatisfiesRelationConstraint(gdd::graph_t const& g, gdd::detail::MappingT const& pg_map,
                                     std::pair<std::size_t, std::string> const& lhs_rel,
                                     gdd::detail::DistanceOperand const& rhs) const;

    bool SatisfiesAttributeConstraint(gdd::graph_t const& g, gdd::detail::MappingT const& pg_map,
                                      gdd::detail::DistanceConstraint const& constraint) const;

    bool SatisfiesConstraint(gdd::graph_t const& g, gdd::detail::MappingT const& pg_map,
                             gdd::detail::DistanceConstraint const& constraint) const;

    bool SatisfiesPhi(gdd::graph_t const& g, gdd::detail::MappingT const& map,
                      Phi const& phi) const {
        return std::ranges::all_of(phi, [this, &g, &map](gdd::detail::DistanceConstraint const& c) {
            return SatisfiesConstraint(g, map, c);
        });
    }

public:
    template <class GraphT, class LhsT, class RhsT>
        requires std::constructible_from<gdd::graph_t, GraphT&&> &&
                         std::constructible_from<Phi, LhsT&&> &&
                         std::constructible_from<Phi, RhsT&&>
    Gdd(GraphT&& pattern, LhsT&& lhs, RhsT&& rhs)
        : pattern_(std::forward<GraphT>(pattern)),
          lhs_(std::forward<LhsT>(lhs)),
          rhs_(std::forward<RhsT>(rhs)) {}

    gdd::graph_t const& GetPattern() const noexcept {
        return pattern_;
    }

    Phi const& GetLhs() const noexcept {
        return lhs_;
    }

    Phi const& GetRhs() const noexcept {
        return rhs_;
    }

    static bool LabelsMatch(std::string const& pattern_label,
                            std::string const& graph_label) noexcept {
        return pattern_label == graph_label;  // TODO: wildcards
    }

    // Testing purposes only
    bool operator==(Gdd const& other) const {
        return std::ranges::is_permutation(lhs_, other.lhs_) &&
               std::ranges::is_permutation(rhs_, other.rhs_) &&
               gdd::detail::IsSubgraph(pattern_, other.pattern_) &&
               gdd::detail::IsSubgraph(other.pattern_, pattern_);
    }

    bool Satisfies(gdd::graph_t const& graph, gdd::detail::MappingT const& map) const {
        if (SatisfiesPhi(graph, map, lhs_)) {
            return SatisfiesPhi(graph, map, rhs_);
        }
        return true;
    }
};

struct GddCounterexampleVertex {
    std::size_t pattern_vertex_id;
    std::string pattern_vertex_label;

    std::size_t graph_vertex_id;
    std::string graph_vertex_label;
    std::unordered_map<std::string, std::string> graph_vertex_attributes;
};

struct GddCounterexample {
    std::size_t gdd_index;
    std::vector<GddCounterexampleVertex> match;
};

GddCounterexample BuildCounterexample(gdd::graph_t const& pattern, gdd::graph_t const& graph,
                                      gdd::detail::MappingT const& mapping);

}  // namespace model
