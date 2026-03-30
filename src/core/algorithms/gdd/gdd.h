#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <variant>

#include "core/algorithms/gdd/gdd_graph_description.h"

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

using ConstValue = std::variant<int64_t, double, std::string>;
using DistanceOperand = std::variant<GddToken, ConstValue>;

enum class DistanceMetric : std::uint8_t { kAbsDiff, kEditDistance };
enum class CmpOp : std::uint8_t { kEq, kLe };

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

}  // namespace gdd::detail

class Gdd {
public:
    using Phi = std::vector<gdd::detail::DistanceConstraint>;

private:
    gdd::graph_t pattern_;
    Phi lhs_;
    Phi rhs_;

    std::optional<gdd::vertex_t> FindPatternVertexById(size_t id) const;

    bool SatisfiesConstraint(gdd::graph_t const& g,
                             std::unordered_map<gdd::vertex_t, gdd::vertex_t> const& map,
                             gdd::detail::DistanceConstraint const& constraint) const;

    bool SatisfiesPhi(gdd::graph_t const& g,
                      std::unordered_map<gdd::vertex_t, gdd::vertex_t> const& map,
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

    // Testing purposes only
    bool operator==(Gdd const& other) const {
        return std::ranges::is_permutation(lhs_, other.lhs_) &&
               std::ranges::is_permutation(rhs_, other.rhs_) &&
               gdd::detail::IsSubgraph(pattern_, other.pattern_) &&
               gdd::detail::IsSubgraph(other.pattern_, pattern_);
    }

    bool Satisfies(gdd::graph_t const& graph,
                   std::unordered_map<gdd::vertex_t, gdd::vertex_t> const& map) const {
        if (SatisfiesPhi(graph, map, lhs_)) {
            return SatisfiesPhi(graph, map, rhs_);
        }
        return true;
    }
};

}  // namespace model
