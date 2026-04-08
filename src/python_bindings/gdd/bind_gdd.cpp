#include "python_bindings/gdd/bind_gdd.h"

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/algorithms/gdd/gdd_validator/gdd_validator.h"
#include "core/algorithms/gdd/gdd_validator/naive_gdd_validator.h"
#include "core/parser/graph_parser/graph_parser.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {

namespace {

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

std::string Repr(model::gdd::detail::AttrTag const& tag) {
    return "AttrTag(name='" + tag.name + "')";
}

std::string Repr(model::gdd::detail::RelTag const& tag) {
    return "RelTag(name='" + tag.name + "')";
}

std::string Repr(model::gdd::detail::GddToken const& token) {
    using model::gdd::detail::AttrTag;
    using model::gdd::detail::RelTag;

    std::string const field_repr = std::visit(Overloaded{
                                                      [](AttrTag const& tag) { return Repr(tag); },
                                                      [](RelTag const& tag) { return Repr(tag); },
                                              },
                                              token.field);

    return "GddToken(pattern_vertex_id=" + std::to_string(token.pattern_vertex_id) +
           ", field=" + field_repr + ")";
}

std::string Repr(model::gdd::detail::ConstValue const& value) {
    using model::gdd::detail::ConstValue;

    return std::visit(Overloaded{
                              [](int64_t v) { return "ConstValue(" + std::to_string(v) + ")"; },
                              [](double v) {
                                  std::ostringstream out;
                                  out << "ConstValue(" << v << ")";
                                  return out.str();
                              },
                              [](std::string const& v) { return "ConstValue('" + v + "')"; },
                      },
                      value);
}

std::string Repr(model::gdd::detail::DistanceOperand const& operand) {
    using model::gdd::detail::ConstValue;
    using model::gdd::detail::GddToken;

    return std::visit(Overloaded{
                              [](GddToken const& token) { return Repr(token); },
                              [](ConstValue const& value) { return Repr(value); },
                      },
                      operand);
}

std::string Repr(model::gdd::detail::DistanceMetric metric) {
    using model::gdd::detail::DistanceMetric;

    switch (metric) {
        case DistanceMetric::kAbsDiff:
            return "DistanceMetric.ABS_DIFF";
        case DistanceMetric::kEditDistance:
            return "DistanceMetric.EDIT_DISTANCE";
    }
    return "DistanceMetric.<unknown>";
}

std::string Repr(model::gdd::detail::CmpOp op) {
    using model::gdd::detail::CmpOp;

    switch (op) {
        case CmpOp::kEq:
            return "CmpOp.EQ";
        case CmpOp::kLe:
            return "CmpOp.LE";
    }
    return "CmpOp.<unknown>";
}

}  // namespace

void BindGdd(pybind11::module_& main_module) {
    namespace py = pybind11;
    using algos::Algorithm;
    using algos::GddValidator;
    using algos::NaiveGddValidator;
    using model::Gdd;
    using model::gdd::graph_t;
    using model::gdd::detail::AttrTag;
    using model::gdd::detail::CmpOp;
    using model::gdd::detail::ConstValue;
    using model::gdd::detail::DistanceConstraint;
    using model::gdd::detail::DistanceMetric;
    using model::gdd::detail::DistanceOperand;
    using model::gdd::detail::GddToken;
    using model::gdd::detail::RelTag;
    using model::gdd::detail::TokenField;
    using py::literals::operator""_a;

    auto gdd_module = main_module.def_submodule("gdd");

    py::enum_<DistanceMetric>(gdd_module, "DistanceMetric")
            .value("ABS_DIFF", DistanceMetric::kAbsDiff)
            .value("EDIT_DISTANCE", DistanceMetric::kEditDistance)
            .export_values()
            .def("__eq__",
                 [](DistanceMetric const& lhs, DistanceMetric const& rhs) { return lhs == rhs; });

    py::enum_<CmpOp>(gdd_module, "CmpOp")
            .value("EQ", CmpOp::kEq)
            .value("LE", CmpOp::kLe)
            .export_values()
            .def("__eq__", [](CmpOp const& lhs, CmpOp const& rhs) { return lhs == rhs; });

    py::class_<AttrTag>(gdd_module, "AttrTag")
            .def(py::init<>())
            .def(py::init<std::string>(), "name"_a)
            .def_readwrite("name", &AttrTag::name)
            .def("__eq__", [](AttrTag const& lhs, AttrTag const& rhs) { return lhs == rhs; })
            .def("__repr__", [](AttrTag const& tag) { return Repr(tag); });

    py::class_<RelTag>(gdd_module, "RelTag")
            .def(py::init<>())
            .def(py::init<std::string>(), "name"_a)
            .def_readwrite("name", &RelTag::name)
            .def("__eq__", [](RelTag const& lhs, RelTag const& rhs) { return lhs == rhs; })
            .def("__repr__", [](RelTag const& tag) { return Repr(tag); });

    py::class_<GddToken>(gdd_module, "GddToken")
            .def(py::init<>())
            .def(py::init<std::size_t, TokenField>(), "pattern_vertex_id"_a, "field"_a)
            .def(py::init([](std::size_t pattern_vertex_id, AttrTag field) {
                     return GddToken{pattern_vertex_id, TokenField{std::move(field)}};
                 }),
                 "pattern_vertex_id"_a, "field"_a)
            .def(py::init([](std::size_t pattern_vertex_id, RelTag field) {
                     return GddToken{pattern_vertex_id, TokenField{std::move(field)}};
                 }),
                 "pattern_vertex_id"_a, "field"_a)
            .def_readwrite("pattern_vertex_id", &GddToken::pattern_vertex_id)
            .def_readwrite("field", &GddToken::field)
            .def("__eq__", [](GddToken const& lhs, GddToken const& rhs) { return lhs == rhs; })
            .def("__repr__", [](GddToken const& token) { return Repr(token); })
            .def("is_attr_tag",
                 [](GddToken const& token) { return std::holds_alternative<AttrTag>(token.field); })
            .def("is_rel_tag",
                 [](GddToken const& token) { return std::holds_alternative<RelTag>(token.field); })
            .def("get_attr_tag",
                 [](GddToken const& token) { return std::get<AttrTag>(token.field); })
            .def("get_rel_tag",
                 [](GddToken const& token) { return std::get<RelTag>(token.field); });

    py::class_<DistanceConstraint>(gdd_module, "DistanceConstraint")
            .def(py::init<>())
            .def(py::init<DistanceOperand, DistanceOperand, double, DistanceMetric, CmpOp>(),
                 "lhs"_a, "rhs"_a, "threshold"_a, "metric"_a, "op"_a)

            .def(py::init([](GddToken lhs, GddToken rhs, double threshold, DistanceMetric metric,
                             CmpOp op) {
                     return DistanceConstraint{
                             DistanceOperand{std::move(lhs)},
                             DistanceOperand{std::move(rhs)},
                             threshold,
                             metric,
                             op,
                     };
                 }),
                 "lhs"_a, "rhs"_a, "threshold"_a, "metric"_a, "op"_a)

            .def(py::init([](GddToken lhs, int64_t rhs, double threshold, DistanceMetric metric,
                             CmpOp op) {
                     return DistanceConstraint{
                             DistanceOperand{std::move(lhs)},
                             DistanceOperand{ConstValue{rhs}},
                             threshold,
                             metric,
                             op,
                     };
                 }),
                 "lhs"_a, "rhs"_a, "threshold"_a, "metric"_a, "op"_a)

            .def(py::init([](GddToken lhs, double rhs, double threshold, DistanceMetric metric,
                             CmpOp op) {
                     return DistanceConstraint{
                             DistanceOperand{std::move(lhs)},
                             DistanceOperand{ConstValue{rhs}},
                             threshold,
                             metric,
                             op,
                     };
                 }),
                 "lhs"_a, "rhs"_a, "threshold"_a, "metric"_a, "op"_a)

            .def(py::init([](GddToken lhs, std::string rhs, double threshold, DistanceMetric metric,
                             CmpOp op) {
                     return DistanceConstraint{
                             DistanceOperand{std::move(lhs)},
                             DistanceOperand{ConstValue{std::move(rhs)}},
                             threshold,
                             metric,
                             op,
                     };
                 }),
                 "lhs"_a, "rhs"_a, "threshold"_a, "metric"_a, "op"_a)

            .def(py::init([](int64_t lhs, GddToken rhs, double threshold, DistanceMetric metric,
                             CmpOp op) {
                     return DistanceConstraint{
                             DistanceOperand{ConstValue{lhs}},
                             DistanceOperand{std::move(rhs)},
                             threshold,
                             metric,
                             op,
                     };
                 }),
                 "lhs"_a, "rhs"_a, "threshold"_a, "metric"_a, "op"_a)

            .def(py::init([](double lhs, GddToken rhs, double threshold, DistanceMetric metric,
                             CmpOp op) {
                     return DistanceConstraint{
                             DistanceOperand{ConstValue{lhs}},
                             DistanceOperand{std::move(rhs)},
                             threshold,
                             metric,
                             op,
                     };
                 }),
                 "lhs"_a, "rhs"_a, "threshold"_a, "metric"_a, "op"_a)

            .def(py::init([](std::string lhs, GddToken rhs, double threshold, DistanceMetric metric,
                             CmpOp op) {
                     return DistanceConstraint{
                             DistanceOperand{ConstValue{std::move(lhs)}},
                             DistanceOperand{std::move(rhs)},
                             threshold,
                             metric,
                             op,
                     };
                 }),
                 "lhs"_a, "rhs"_a, "threshold"_a, "metric"_a, "op"_a)

            .def(py::init([](int64_t lhs, int64_t rhs, double threshold, DistanceMetric metric,
                             CmpOp op) {
                     return DistanceConstraint{
                             DistanceOperand{ConstValue{lhs}},
                             DistanceOperand{ConstValue{rhs}},
                             threshold,
                             metric,
                             op,
                     };
                 }),
                 "lhs"_a, "rhs"_a, "threshold"_a, "metric"_a, "op"_a)

            .def(py::init([](double lhs, double rhs, double threshold, DistanceMetric metric,
                             CmpOp op) {
                     return DistanceConstraint{
                             DistanceOperand{ConstValue{lhs}},
                             DistanceOperand{ConstValue{rhs}},
                             threshold,
                             metric,
                             op,
                     };
                 }),
                 "lhs"_a, "rhs"_a, "threshold"_a, "metric"_a, "op"_a)

            .def(py::init([](std::string lhs, std::string rhs, double threshold,
                             DistanceMetric metric, CmpOp op) {
                     return DistanceConstraint{
                             DistanceOperand{ConstValue{std::move(lhs)}},
                             DistanceOperand{ConstValue{std::move(rhs)}},
                             threshold,
                             metric,
                             op,
                     };
                 }),
                 "lhs"_a, "rhs"_a, "threshold"_a, "metric"_a, "op"_a)

            .def_readwrite("lhs", &DistanceConstraint::lhs)
            .def_readwrite("rhs", &DistanceConstraint::rhs)
            .def_readwrite("threshold", &DistanceConstraint::threshold)
            .def_readwrite("metric", &DistanceConstraint::metric)
            .def_readwrite("op", &DistanceConstraint::op)

            .def("__eq__", [](DistanceConstraint const& lhs,
                              DistanceConstraint const& rhs) { return lhs == rhs; })
            .def("__repr__", [](DistanceConstraint const& c) {
                return "DistanceConstraint(lhs=" + Repr(c.lhs) + ", rhs=" + Repr(c.rhs) +
                       ", threshold=" + std::to_string(c.threshold) + ", metric=" + Repr(c.metric) +
                       ", op=" + Repr(c.op) + ")";
            });

    gdd_module.def(
            "AttrConst",
            [](std::size_t pid, std::string attr, int64_t c, DistanceMetric metric, CmpOp op,
               double t) {
                return DistanceConstraint{
                        .lhs = DistanceOperand{GddToken{pid, AttrTag{std::move(attr)}}},
                        .rhs = DistanceOperand{ConstValue{c}},
                        .threshold = t,
                        .metric = metric,
                        .op = op,
                };
            },
            "pid"_a, "attr"_a, "c"_a, "metric"_a, "op"_a, "threshold"_a);

    gdd_module.def(
            "AttrConst",
            [](std::size_t pid, std::string attr, double c, DistanceMetric metric, CmpOp op,
               double t) {
                return DistanceConstraint{
                        .lhs = DistanceOperand{GddToken{pid, AttrTag{std::move(attr)}}},
                        .rhs = DistanceOperand{ConstValue{c}},
                        .threshold = t,
                        .metric = metric,
                        .op = op,
                };
            },
            "pid"_a, "attr"_a, "c"_a, "metric"_a, "op"_a, "threshold"_a);

    gdd_module.def(
            "AttrConst",
            [](std::size_t pid, std::string attr, std::string c, DistanceMetric metric, CmpOp op,
               double t) {
                return DistanceConstraint{
                        .lhs = DistanceOperand{GddToken{pid, AttrTag{std::move(attr)}}},
                        .rhs = DistanceOperand{ConstValue{std::move(c)}},
                        .threshold = t,
                        .metric = metric,
                        .op = op,
                };
            },
            "pid"_a, "attr"_a, "c"_a, "metric"_a, "op"_a, "threshold"_a);

    gdd_module.def(
            "AttrAttr",
            [](std::size_t pid1, std::string a1, std::size_t pid2, std::string a2,
               DistanceMetric metric, CmpOp op, double t) {
                return DistanceConstraint{
                        .lhs = DistanceOperand{GddToken{pid1, AttrTag{std::move(a1)}}},
                        .rhs = DistanceOperand{GddToken{pid2, AttrTag{std::move(a2)}}},
                        .threshold = t,
                        .metric = metric,
                        .op = op,
                };
            },
            "pid1"_a, "a1"_a, "pid2"_a, "a2"_a, "metric"_a, "op"_a, "threshold"_a);

    gdd_module.def(
            "RelConst",
            [](std::size_t pid, std::string rela, int64_t cr) {
                return DistanceConstraint{
                        .lhs = DistanceOperand{GddToken{pid, RelTag{std::move(rela)}}},
                        .rhs = DistanceOperand{ConstValue{cr}},
                        .threshold = 0.0,
                        .metric = DistanceMetric::kAbsDiff,
                        .op = CmpOp::kEq,
                };
            },
            "pid"_a, "relation"_a, "const_value"_a);

    gdd_module.def(
            "RelConst",
            [](std::size_t pid, std::string rela, double cr) {
                return DistanceConstraint{
                        .lhs = DistanceOperand{GddToken{pid, RelTag{std::move(rela)}}},
                        .rhs = DistanceOperand{ConstValue{cr}},
                        .threshold = 0.0,
                        .metric = DistanceMetric::kAbsDiff,
                        .op = CmpOp::kEq,
                };
            },
            "pid"_a, "relation"_a, "const_value"_a);

    gdd_module.def(
            "RelConst",
            [](std::size_t pid, std::string rela, std::string cr) {
                return DistanceConstraint{
                        .lhs = DistanceOperand{GddToken{pid, RelTag{std::move(rela)}}},
                        .rhs = DistanceOperand{ConstValue{std::move(cr)}},
                        .threshold = 0.0,
                        .metric = DistanceMetric::kAbsDiff,
                        .op = CmpOp::kEq,
                };
            },
            "pid"_a, "relation"_a, "const_value"_a);

    gdd_module.def(
            "RelRel",
            [](std::size_t pid1, std::string rela1, std::size_t pid2, std::string rela2) {
                return DistanceConstraint{
                        .lhs = DistanceOperand{GddToken{pid1, RelTag{std::move(rela1)}}},
                        .rhs = DistanceOperand{GddToken{pid2, RelTag{std::move(rela2)}}},
                        .threshold = 0.0,
                        .metric = DistanceMetric::kAbsDiff,
                        .op = CmpOp::kEq,
                };
            },
            "pid1"_a, "relation1"_a, "pid2"_a, "relation2"_a);

    py::class_<Gdd>(gdd_module, "Gdd")
            .def(py::init([](std::string const& pattern_dot, std::vector<DistanceConstraint> lhs,
                             std::vector<DistanceConstraint> rhs) {
                     std::stringstream ss(pattern_dot);
                     graph_t pattern = parser::graph_parser::gdd::ReadGraph(ss);
                     return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
                 }),
                 "pattern_dot"_a, "lhs"_a, "rhs"_a)

            .def_property_readonly("lhs", [](Gdd const& gdd) { return gdd.GetLhs(); })
            .def_property_readonly("rhs", [](Gdd const& gdd) { return gdd.GetRhs(); })

            .def("__repr__",
                 [](Gdd const& gdd) {
                     return "Gdd(lhs_size=" + std::to_string(gdd.GetLhs().size()) +
                            ", rhs_size=" + std::to_string(gdd.GetRhs().size()) + ")";
                 })
            .def("__eq__", [](Gdd const& lhs, Gdd const& rhs) { return lhs == rhs; });

    gdd_module.def(
            "GddFromDot",
            [](std::string const& pattern_dot, std::vector<DistanceConstraint> lhs,
               std::vector<DistanceConstraint> rhs) {
                std::stringstream ss(pattern_dot);
                graph_t pattern = parser::graph_parser::gdd::ReadGraph(ss);
                return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
            },
            "pattern_dot"_a, "lhs"_a, "rhs"_a);

    gdd_module.def(
            "GddFromDotFile",
            [](std::filesystem::path const& pattern_dot_file, std::vector<DistanceConstraint> lhs,
               std::vector<DistanceConstraint> rhs) {
                graph_t pattern = parser::graph_parser::gdd::ReadGraph(pattern_dot_file);
                return Gdd(std::move(pattern), std::move(lhs), std::move(rhs));
            },
            "pattern_dot_file"_a, "lhs"_a, "rhs"_a);

    auto const gdd_algos_module =
            BindPrimitive<NaiveGddValidator>(gdd_module, &GddValidator::GetResult, "GddValidator",
                                             "get_result", {"NaiveGddValidator"});

    gdd_module.attr("Default") = gdd_algos_module.attr("Default");
}

}  // namespace python_bindings
