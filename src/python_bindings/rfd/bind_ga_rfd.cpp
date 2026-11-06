#include <pybind11/pybind11.h>

#include <algorithm>
#include <string>
#include <vector>

#include <pybind11/stl.h>

#include "core/algorithms/rfd/distance_metric.h"
#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/config/exceptions.h"
#include "core/util/custom_metric/custom_metric.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace py = pybind11;
using algos::rfd::GaRfd;
using algos::rfd::RFD;

namespace python_bindings {

namespace {

std::vector<std::string> LhsNames(RFD const& rfd) {
    std::vector<std::string> names;
    names.reserve(rfd.lhs_indices.size());
    for (std::size_t index : rfd.lhs_indices) {
        names.push_back(rfd.input_table ? rfd.input_table->GetColumnName(index)
                                        : std::to_string(index));
    }
    return names;
}

std::string RhsName(RFD const& rfd) {
    return rfd.input_table ? rfd.input_table->GetColumnName(rfd.rhs_index)
                           : std::to_string(rfd.rhs_index);
}

std::string ToStringNoTable(RFD const& rfd) {
    std::string result = "[";
    for (std::size_t i = 0; i < rfd.lhs_indices.size(); ++i) {
        if (i != 0) result += ", ";
        result += std::to_string(rfd.lhs_indices[i]);
    }
    return result + "] -> [" + std::to_string(rfd.rhs_index) +
           "] (conf=" + std::to_string(rfd.confidence) + ", supp=" + std::to_string(rfd.support) +
           ")";
}

}  // namespace

void BindGaRfd(py::module_& main_module) {
    auto rfd_module = main_module.def_submodule("rfd");

    rfd_module.def("levenshtein_metric", &algos::rfd::LevenshteinMetric);
    rfd_module.def("equality_metric", &algos::rfd::EqualityMetric);
    rfd_module.def("abs_diff_metric", &algos::rfd::AbsoluteDifferenceMetric);
    rfd_module.def("abs_threshold_metric", &algos::rfd::AbsoluteThresholdMetricFactory,
                   py::arg("tolerance"));

    py::class_<RFD>(rfd_module, "RFD")
            .def(py::init<>())
            .def(py::init<std::vector<std::size_t>, std::size_t, double, double>(),
                 py::arg("lhs_indices"), py::arg("rhs_index"), py::arg("support") = 0.0,
                 py::arg("confidence") = 0.0)
            .def_property_readonly("lhs_indices", [](RFD const& r) { return r.lhs_indices; })
            .def_property_readonly("rhs_index", [](RFD const& r) { return r.rhs_index; })
            .def_property_readonly("lhs", &LhsNames)
            .def_property_readonly("rhs", &RhsName)
            .def_property_readonly("support", [](RFD const& r) { return r.support; })
            .def_property_readonly("confidence", [](RFD const& r) { return r.confidence; })
            .def("__str__",
                 [](RFD const& r) { return r.input_table ? r.ToString() : ToStringNoTable(r); })
            .def("__repr__",
                 [](RFD const& r) {
                     std::string repr = "lhs_indices=[";
                     for (std::size_t i = 0; i < r.lhs_indices.size(); ++i) {
                         if (i != 0) repr += ", ";
                         repr += std::to_string(r.lhs_indices[i]);
                     }
                     repr += "], rhs_index=" + std::to_string(r.rhs_index) +
                             ", support=" + std::to_string(r.support) +
                             ", confidence=" + std::to_string(r.confidence) + ")";
                     return repr;
                 })
            .def("__eq__", [](RFD const& a, RFD const& b) { return a == b; })
            .def("__ne__", [](RFD const& a, RFD const& b) { return a != b; })
            .def("__lt__", [](RFD const& a, RFD const& b) { return a < b; })
            .def("__le__", [](RFD const& a, RFD const& b) { return a <= b; })
            .def("__gt__", [](RFD const& a, RFD const& b) { return a > b; })
            .def("__ge__", [](RFD const& a, RFD const& b) { return a >= b; })
            // .def("__hash__", [](RFD const& r) { return
            // py::hash(py::make_tuple(VectorToTuple(r.lhs_indices), r.rhs_index)); })
            .def(py::pickle(
                    [](RFD const& rfd) {
                        return py::make_tuple(rfd.lhs_indices, rfd.rhs_index, rfd.support,
                                              rfd.confidence);
                    },
                    [](py::tuple state) {
                        if (state.size() != 4) {
                            throw std::runtime_error("Invalid state for RFD pickle");
                        }
                        return RFD(state[0].cast<std::vector<std::size_t>>(),
                                   state[1].cast<std::size_t>(), state[2].cast<double>(),
                                   state[3].cast<double>());
                    }));

    BindPrimitiveNoBase<GaRfd>(rfd_module, "GaRfd").def("get_rfds", &GaRfd::GetRfds);
}

}  // namespace python_bindings
