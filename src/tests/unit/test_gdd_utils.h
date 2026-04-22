#pragma once

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/range/iterator_range.hpp>
#include <gtest/gtest.h>

#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/parser/graph_parser/graph_parser.h"

namespace tests {

inline std::string SanitizeParamName(std::string name) {
    for (char& ch : name) {
        if (!std::isalnum(static_cast<unsigned char>(ch))) {
            ch = '_';
        }
    }
    return name;
}

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

// Graph utility

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

// Pattern builders

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

inline graph_t MakePatternPersonCity(std::string const& edge_label = "lives_in") {
    return ReadGraphFromDot(std::string(R"(digraph P {
        0 [label="Person"];
        1 [label="City"];
        0 -> 1 [label=")") + edge_label +
                            R"("];
    })");
}

inline graph_t MakePatternCityCountry(std::string const& edge_label = "in_country") {
    return ReadGraphFromDot(std::string(R"(digraph P {
        0 [label="City"];
        1 [label="Country"];
        0 -> 1 [label=")") + edge_label +
                            R"("];
    })");
}

inline graph_t MakePatternCompanyCity(std::string const& edge_label = "hq_in") {
    return ReadGraphFromDot(std::string(R"(digraph P {
        0 [label="Company"];
        1 [label="City"];
        0 -> 1 [label=")") + edge_label +
                            R"("];
    })");
}

// Distance constraint builders

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

// Graphs and gdds for validator testing

inline Gdd MakeGddMishaLivesInAmsterdam() {
    auto pattern = MakePatternPersonCity("lives_in");
    Gdd::Phi lhs{EqStrAttrToConst(0, "name", "Misha")};
    Gdd::Phi rhs{EqStrAttrToConst(1, "name", "Amsterdam")};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

inline Gdd MakeGddRigaInLatvia() {
    auto pattern = MakePatternCityCountry("in_country");
    Gdd::Phi lhs{EqStrAttrToConst(0, "name", "Riga")};
    Gdd::Phi rhs{EqStrAttrToConst(1, "name", "Latvia")};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

inline Gdd MakeGddVacuousImplicationNonexistentPerson() {
    auto pattern = MakePatternPersonCity("lives_in");
    Gdd::Phi lhs{EqStrAttrToConst(0, "name", "Nonexistent")};
    Gdd::Phi rhs{EqStrAttrToConst(1, "name", "Nowhere")};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

inline Gdd MakeGddCompanyHqInAmsterdamNoCompanyInGraph() {
    auto pattern = MakePatternCompanyCity("hq_in");
    Gdd::Phi lhs{};
    Gdd::Phi rhs{EqStrAttrToConst(1, "name", "Amsterdam")};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

inline Gdd MakeGddPersonAge25ImpliesLivesInAmsterdamAsRelationConstraint() {
    auto pattern = MakePatternPersonCity("lives_in");
    Gdd::Phi lhs{AbsDiffLeAttrToConst(0, "age", model::gdd::detail::ConstValue{25LL}, 0.0)};
    Gdd::Phi rhs{RelToConst(0, "lives_in", 101)};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

inline Gdd MakeGddLabelConstraintPersonImpliesCityLabelCloseToCity() {
    auto pattern = MakePatternPersonCity("lives_in");
    Gdd::Phi lhs{EqStrAttrToConst(0, "name", "Misha")};
    Gdd::Phi rhs{EditLeStrAttrToConst(1, "label", "Coty", 1.0)};
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

inline std::string LargeGraphAllGoodDot() {
    return R"(digraph G {
        1 [label="Person", name="Misha", age="25", email="m@x"];
        2 [label="Person", name="Bob",   age="31"];
        3 [label="Person", name="Alice", age="22"];

        101 [label="City", name="Amsterdam", population="821752"];
        102 [label="City", name="Riga",      population="605273"];
        103 [label="City", name="Paris"];

        201 [label="Country", name="Netherlands"];
        202 [label="Country", name="Latvia"];
        203 [label="Country", name="France"];

        1 -> 101 [label="lives_in"];
        2 -> 102 [label="lives_in"];
        3 -> 103 [label="lives_in"];

        101 -> 201 [label="in_country"];
        102 -> 202 [label="in_country"];
        103 -> 203 [label="in_country"];

        1 -> 2 [label="friend"];
        2 -> 3 [label="friend"];
        3 -> 1 [label="friend"];
        101 -> 102 [label="sister_city"];
        102 -> 103 [label="sister_city"];
    })";
}

inline std::string LargeGraphWithViolationMishaAlsoLivesInRigaDot() {
    return R"(digraph G {
        1 [label="Person", name="Misha", age="25", email="m@x"];
        2 [label="Person", name="Bob",   age="31"];
        3 [label="Person", name="Alice", age="22"];

        101 [label="City", name="Amsterdam", population="821752"];
        102 [label="City", name="Riga",      population="605273"];
        103 [label="City", name="Paris"];

        201 [label="Country", name="Netherlands"];
        202 [label="Country", name="Latvia"];
        203 [label="Country", name="France"];

        1 -> 101 [label="lives_in"];
        1 -> 102 [label="lives_in"];
        2 -> 102 [label="lives_in"];
        3 -> 103 [label="lives_in"];

        101 -> 201 [label="in_country"];
        102 -> 202 [label="in_country"];
        103 -> 203 [label="in_country"];

        1 -> 2 [label="friend"];
        2 -> 3 [label="friend"];
        3 -> 1 [label="friend"];
    })";
}

inline std::string GraphNoMatchesForCompanyCityDot() {
    return R"(digraph G {
        1 [label="Person", name="Misha"];
        2 [label="Person", name="Bob"];
        1 -> 2 [label="friend"];
    })";
}

inline std::string DblpLikeGraphDot() {
    return R"(digraph G {
        1 [label="Author", name="Jiawei Han", canonical_author_id="author:han_jiawei"];
        2 [label="Author", name="J. Han",     canonical_author_id="author:han_jiawei"];

        3 [label="Author", name="Philip S. Yu", canonical_author_id="author:yu_philip"];

        4 [label="Author", name="Yi Zhang", canonical_author_id="author:zhang_yi"];
        5 [label="Author", name="Yu Zhang", canonical_author_id="author:zhang_yu"];

        101 [label="Paper", title="Mining Frequent Patterns",     year="2000"];
        102 [label="Paper", title="Mining Frequent Pattern Sets", year="2000"];
        103 [label="Paper", title="Scalable Pattern Search",      year="2023"];
        104 [label="Paper", title="Efficient Pattern Search",     year="2023"];

        201 [label="Venue", name="SIGMOD"];
        202 [label="Venue", name="KDD"];

        1 -> 101 [label="authored"];
        3 -> 101 [label="authored"];

        2 -> 102 [label="authored"];
        3 -> 102 [label="authored"];

        4 -> 103 [label="authored"];
        5 -> 104 [label="authored"];

        101 -> 201 [label="published_in"];
        102 -> 201 [label="published_in"];
        103 -> 202 [label="published_in"];
        104 -> 202 [label="published_in"];
    })";
}

inline graph_t PatternDblpStrong() {
    return ReadGraphFromDot(R"(digraph P {
        0 [label="Author"];
        1 [label="Author"];
        2 [label="Paper"];
        3 [label="Paper"];
        4 [label="Author"];
        5 [label="Venue"];

        0 -> 2 [label="authored"];
        1 -> 3 [label="authored"];
        4 -> 2 [label="authored"];
        4 -> 3 [label="authored"];
        2 -> 5 [label="published_in"];
        3 -> 5 [label="published_in"];
    })");
}

inline graph_t PatternDblpWeak() {
    return ReadGraphFromDot(R"(digraph P {
        0 [label="Author"];
        1 [label="Author"];
        2 [label="Paper"];
        3 [label="Paper"];
        4 [label="Venue"];

        0 -> 2 [label="authored"];
        1 -> 3 [label="authored"];
        2 -> 4 [label="published_in"];
        3 -> 4 [label="published_in"];
    })");
}

inline Gdd MakeGddDblpStrongAuthorResolution() {
    auto pattern = PatternDblpStrong();
    Gdd::Phi lhs{
            EditLeAttrToAttr(0, "name", 1, "name", 8.0),
            EditLeAttrToAttr(2, "year", 3, "year", 0.0),
    };
    Gdd::Phi rhs{
            EditLeAttrToAttr(0, "canonical_author_id", 1, "canonical_author_id", 0.0),
    };
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

inline Gdd MakeGddDblpWeakAuthorResolution() {
    auto pattern = PatternDblpWeak();
    Gdd::Phi lhs{
            EditLeAttrToAttr(0, "name", 1, "name", 2.0),
            EditLeAttrToAttr(2, "year", 3, "year", 0.0),
    };
    Gdd::Phi rhs{
            EditLeAttrToAttr(0, "canonical_author_id", 1, "canonical_author_id", 0.0),
    };
    return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
}

}  // namespace tests
