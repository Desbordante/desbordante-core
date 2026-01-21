#include "python_bindings/dd/bind_dd_verification.h"

#include <cstddef>
#include <functional>
#include <string_view>
#include <unordered_map>

#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/dd_verifier/Metric.h"
#include "core/algorithms/dd/dd_verifier/dd_verifier.h"
#include "core/algorithms/dd/dd_verifier/highlight.h"
#include "core/util/create_dd.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings {
namespace py = pybind11;

void BindDDVerification(py::module_ &main_module) {
    using namespace algos;
    auto dd_verification_module = main_module.def_submodule("dd_verification");
    dd_verification_module
            .def(
                    "make_metric_ptr",
                    [](py::tuple metric_tuple) -> Metric const * {
                        if (py::len(metric_tuple) != 2) {
                            throw std::runtime_error(
                                    "Input must be a tuple of (function, type_string)");
                        }

                        std::string type_tag = metric_tuple[1].cast<std::string>();

                        if (type_tag == "int") {
                            return new FuncMetric<int>(std::move(
                                    metric_tuple[0].cast<std::function<double(int, int)>>()));
                        } else if (type_tag == "double") {
                            return new FuncMetric<double>(std::move(
                                    metric_tuple[0].cast<std::function<double(double, double)>>()));
                        } else if (type_tag == "string") {
                            return new FuncMetric<std::string>(
                                    std::move(metric_tuple[0]
                                                      .cast<std::function<double(std::string,
                                                                                 std::string)>>()));
                        } else {
                            throw std::runtime_error(
                                    "Unsupported type tag. Use 'int', 'double', or 'string'.");
                        }
                    },
                    py::return_value_policy::take_ownership)
            .def("test", [](py::handle map) {
                std::unordered_map<std::string, Metric const *> metrics_map =
                        map.cast<std::unordered_map<std::string, Metric const *>>();
                int a = 1;
                std::byte const *bytes = reinterpret_cast<std::byte const *>(&a);
                return metrics_map["salary_level"]->Dist(bytes, bytes);
            });
    py::class_<model::DFStringConstraint>(dd_verification_module, "DF")
            .def(py::init(&util::dd::CreateDf))
            .def("__str__", &model::DFStringConstraint::ToString)
            .def_property_readonly(
                    "attribute_name",
                    [](model::DFStringConstraint const &DF) { return DF.column_name; })
            .def_property_readonly(
                    "lower_bound",
                    [](model::DFStringConstraint const &DF) { return DF.constraint.lower_bound; })
            .def_property_readonly(
                    "upper_bound",
                    [](model::DFStringConstraint const &DF) { return DF.constraint.upper_bound; })
            .def(pybind11::self == pybind11::self)
            .def(pybind11::self != pybind11::self)
            .def("__hash__", &model::DFStringConstraint::hash)
            .def("to_json", &model::DFStringConstraint::to_JSON);
    py::class_<Highlight>(dd_verification_module, "Highlight")
            .def_property_readonly("attribute_index", &Highlight::GetAttributeIndex)
            .def_property_readonly("distance", &Highlight::GetDistance)
            .def_property_readonly("pair_rows", &Highlight::GetPairRows);
    py::class_<Metric>(dd_verification_module, "Metric");
    py::class_<FuncMetric<int>, Metric>(dd_verification_module, "MetricInt")
            .def(py::init<std::function<double(int, int)>>());
    py::class_<FuncMetric<std::string>, Metric>(dd_verification_module, "MetricStr")
            .def(py::init<std::function<double(std::string, std::string)>>());
    py::class_<FuncMetric<double>, Metric>(dd_verification_module, "MetricDouble")
            .def(py::init<std::function<double(double, double)>>());
    BindPrimitiveNoBase<dd::DDVerifier>(dd_verification_module, "DDVerifier")
            .def("dd_holds", &dd::DDVerifier::DDHolds)
            .def("get_error", &dd::DDVerifier::GetError)
            .def("get_num_error_pairs", &dd::DDVerifier::GetNumErrorRhs)
            .def("get_highlights", &dd::DDVerifier::GetHighlights);
}
}  // namespace python_bindings
