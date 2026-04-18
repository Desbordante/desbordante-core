#pragma once

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/graph/graph_traits.hpp>
#include <boost/range/iterator_range.hpp>
#include <gtest/gtest.h>

#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/parser/graph_parser/graph_parser.h"

namespace tests {

using model::Gdd;
using model::gdd::graph_t;
using model::gdd::vertex_t;

using model::gdd::detail::AttrTag;
using model::gdd::detail::CmpOp;
using model::gdd::detail::ConstValue;
using model::gdd::detail::DistanceConstraint;
using model::gdd::detail::DistanceMetric;
using model::gdd::detail::GddToken;
using model::gdd::detail::RelTag;

inline graph_t ReadGraphFromDot(std::string const& dot) {
    std::stringstream ss;
    ss << dot;
    return parser::graph_parser::gdd::ReadGraph(ss);
}

inline vertex_t AddVertex(graph_t& g, std::uint64_t id, std::string label,
                          std::unordered_map<std::string, std::string> attrs = {}) {
    auto const v = boost::add_vertex(g);
    g[v].id = id;
    g[v].label = std::move(label);
    g[v].attributes = std::move(attrs);
    return v;
}

inline void AddEdge(graph_t& g, vertex_t from, vertex_t to, std::string label) {
    auto [e, ok] = boost::add_edge(from, to, g);
    ASSERT_TRUE(ok);
    g[e].label = std::move(label);
}

inline vertex_t FindVertexById(graph_t const& g, std::uint64_t id) {
    for (auto const v : boost::make_iterator_range(boost::vertices(g))) {
        if (g[v].id == id) {
            return v;
        }
    }
    throw std::runtime_error("vertex with requested id not found");
}

inline vertex_t FindPatternVertex(graph_t const& pattern, std::uint64_t id) {
    return FindVertexById(pattern, id);
}

inline graph_t MakeSingleVertexPattern(std::uint64_t id, std::string label) {
    graph_t g;
    AddVertex(g, id, std::move(label));
    return g;
}

inline graph_t MakeTwoVertexPattern(std::uint64_t id1, std::uint64_t id2, std::string label1,
                                    std::string label2) {
    graph_t g;
    AddVertex(g, id1, std::move(label1));
    AddVertex(g, id2, std::move(label2));
    return g;
}

inline graph_t MakePersonCityPattern(bool reverse_insertion_order = false, int id_shift = 0,
                                     std::string edge_label = "lives_in",
                                     std::string person_name = "Misha",
                                     std::string city_name = "Amsterdam") {
    graph_t g;

    vertex_t v_person{};
    vertex_t v_city{};

    if (!reverse_insertion_order) {
        v_person = AddVertex(g, static_cast<std::uint64_t>(1 + id_shift), "Person",
                             {{"name", std::move(person_name)}});
        v_city = AddVertex(g, static_cast<std::uint64_t>(2 + id_shift), "City",
                           {{"name", std::move(city_name)}});
    } else {
        v_city = AddVertex(g, static_cast<std::uint64_t>(2 + id_shift), "City",
                           {{"name", std::move(city_name)}});
        v_person = AddVertex(g, static_cast<std::uint64_t>(1 + id_shift), "Person",
                             {{"name", std::move(person_name)}});
    }

    AddEdge(g, v_person, v_city, std::move(edge_label));
    return g;
}

inline graph_t PatternPersonCity(std::string const& edge_label = "lives_in") {
    return ReadGraphFromDot(std::string(R"(digraph P {
        0 [label="Person"];
        1 [label="City"];
        0 -> 1 [label=")") + edge_label +
                            R"("];
    })");
}

inline graph_t PatternCityCountry(std::string const& edge_label = "in_country") {
    return ReadGraphFromDot(std::string(R"(digraph P {
        0 [label="City"];
        1 [label="Country"];
        0 -> 1 [label=")") + edge_label +
                            R"("];
    })");
}

inline graph_t PatternCompanyCity(std::string const& edge_label = "hq_in") {
    return ReadGraphFromDot(std::string(R"(digraph P {
        0 [label="Company"];
        1 [label="City"];
        0 -> 1 [label=")") + edge_label +
                            R"("];
    })");
}

inline DistanceConstraint AttrConst(std::size_t pid, std::string attr_name, ConstValue value,
                                    DistanceMetric metric, CmpOp op, double threshold) {
    return DistanceConstraint{
            .lhs = GddToken{pid, AttrTag{std::move(attr_name)}},
            .rhs = std::move(value),
            .threshold = threshold,
            .metric = metric,
            .op = op,
    };
}

inline DistanceConstraint AttrAttr(std::size_t pid1, std::string attr1, std::size_t pid2,
                                   std::string attr2, DistanceMetric metric, CmpOp op,
                                   double threshold) {
    return DistanceConstraint{
            .lhs = GddToken{pid1, AttrTag{std::move(attr1)}},
            .rhs = GddToken{pid2, AttrTag{std::move(attr2)}},
            .threshold = threshold,
            .metric = metric,
            .op = op,
    };
}

inline DistanceConstraint RelConst(std::size_t pid, std::string rel_name, ConstValue target) {
    return DistanceConstraint{
            .lhs = GddToken{pid, RelTag{std::move(rel_name)}},
            .rhs = std::move(target),
            .threshold = 0.0,
            .metric = DistanceMetric::kAbsDiff,
            .op = CmpOp::kEq,
    };
}

inline DistanceConstraint RelRel(std::size_t pid1, std::string rel1, std::size_t pid2,
                                 std::string rel2) {
    return DistanceConstraint{
            .lhs = GddToken{pid1, RelTag{std::move(rel1)}},
            .rhs = GddToken{pid2, RelTag{std::move(rel2)}},
            .threshold = 0.0,
            .metric = DistanceMetric::kAbsDiff,
            .op = CmpOp::kEq,
    };
}

inline DistanceConstraint EqStrAttrToConst(std::size_t pattern_vid, std::string attr_name,
                                           std::string value) {
    return DistanceConstraint{
            .lhs = GddToken{pattern_vid, AttrTag{std::move(attr_name)}},
            .rhs = ConstValue{std::move(value)},
            .threshold = 0.0,
            .metric = DistanceMetric::kEditDistance,
            .op = CmpOp::kEq,
    };
}

inline DistanceConstraint EditLeAttrToAttr(std::size_t lhs_pattern_vid, std::string lhs_attr_name,
                                           std::size_t rhs_pattern_vid, std::string rhs_attr_name,
                                           double threshold) {
    return DistanceConstraint{
            .lhs = GddToken{lhs_pattern_vid, AttrTag{std::move(lhs_attr_name)}},
            .rhs = GddToken{rhs_pattern_vid, AttrTag{std::move(rhs_attr_name)}},
            .threshold = threshold,
            .metric = DistanceMetric::kEditDistance,
            .op = CmpOp::kLe,
    };
}

inline DistanceConstraint EditLeStrAttrToConst(std::size_t pattern_vid, std::string attr_name,
                                               std::string value, double threshold) {
    return DistanceConstraint{
            .lhs = GddToken{pattern_vid, AttrTag{std::move(attr_name)}},
            .rhs = ConstValue{std::move(value)},
            .threshold = threshold,
            .metric = DistanceMetric::kEditDistance,
            .op = CmpOp::kLe,
    };
}

inline DistanceConstraint AbsDiffLeAttrToConst(std::size_t pattern_vid, std::string attr_name,
                                               ConstValue value, double threshold) {
    return DistanceConstraint{
            .lhs = GddToken{pattern_vid, AttrTag{std::move(attr_name)}},
            .rhs = std::move(value),
            .threshold = threshold,
            .metric = DistanceMetric::kAbsDiff,
            .op = CmpOp::kLe,
    };
}

inline DistanceConstraint RelToConst(std::size_t pattern_vid, std::string rel_name,
                                     std::int64_t target_id) {
    return DistanceConstraint{
            .lhs = GddToken{pattern_vid, RelTag{std::move(rel_name)}},
            .rhs = ConstValue{target_id},
            .threshold = 0.0,
            .metric = DistanceMetric::kAbsDiff,
            .op = CmpOp::kEq,
    };
}

inline DistanceConstraint MakeAttrEditDistanceLe(std::size_t pattern_vid, std::string attr_name,
                                                 std::string value, double threshold) {
    return DistanceConstraint{
            .lhs = GddToken{pattern_vid, AttrTag{std::move(attr_name)}},
            .rhs = ConstValue{std::move(value)},
            .threshold = threshold,
            .metric = DistanceMetric::kEditDistance,
            .op = CmpOp::kLe,
    };
}

}  // namespace tests::gdd_test_utils
