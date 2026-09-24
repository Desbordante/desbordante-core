#include <pybind11/pybind11.h>

#include <algorithm>

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

RFD MakeRfd(std::vector<std::string> lhs, std::string rhs, std::vector<std::size_t> lhs_indices,
            std::size_t rhs_index, double support, double confidence) {
    if (lhs.empty()) throw config::ConfigurationError("LHS must not be empty");
    if (lhs.size() != lhs_indices.size()) {
        throw config::ConfigurationError("lhs and lhs_indices sizes must match");
    }
    if (rhs.empty()) throw config::ConfigurationError("RHS must name a column");
    if (std::find(lhs_indices.begin(), lhs_indices.end(), rhs_index) != lhs_indices.end()) {
        throw config::ConfigurationError("RHS attribute cannot be in LHS");
    }
    RFD rfd;
    rfd.lhs = std::move(lhs);
    rfd.rhs = std::move(rhs);
    rfd.lhs_indices = std::move(lhs_indices);
    rfd.rhs_index = rhs_index;
    rfd.support = support;
    rfd.confidence = confidence;
    return rfd;
}

std::size_t HashRfd(RFD const& rfd) {
    std::size_t hash = rfd.rhs_index + 0x9e3779b9u;
    for (std::size_t index : rfd.lhs_indices) {
        hash ^= index + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    }
    return hash;
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
            .def(py::init(&MakeRfd), py::arg("lhs"), py::arg("rhs"), py::arg("lhs_indices"),
                 py::arg("rhs_index"), py::arg("support") = 0.0, py::arg("confidence") = 0.0)
            .def_property_readonly("lhs", [](RFD const& r) { return r.lhs; })
            .def_property_readonly("rhs", [](RFD const& r) { return r.rhs; })
            .def_property_readonly("lhs_indices", [](RFD const& r) { return r.lhs_indices; })
            .def_property_readonly("rhs_index", [](RFD const& r) { return r.rhs_index; })
            .def_property_readonly("support", [](RFD const& r) { return r.support; })
            .def_property_readonly("confidence", [](RFD const& r) { return r.confidence; })
            .def("__str__", &RFD::ToString)
            .def("__repr__",
                 [](RFD const& r) {
                     std::string repr = "RFD(lhs=[";
                     for (std::size_t i = 0; i < r.lhs.size(); ++i) {
                         if (i != 0) repr += ", ";
                         repr += "'" + r.lhs[i] + "'";
                     }
                     repr += "], rhs='" + r.rhs + "', lhs_indices=[";
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
            .def("__hash__", [](RFD const& r) { return HashRfd(r); })
            .def(py::pickle(
                    [](RFD const& rfd) {
                        return py::make_tuple(rfd.lhs, rfd.rhs, rfd.lhs_indices, rfd.rhs_index,
                                              rfd.support, rfd.confidence);
                    },
                    [](py::tuple state) {
                        if (state.size() != 6) {
                            throw std::runtime_error("Invalid state for RFD pickle");
                        }
                        return MakeRfd(state[0].cast<std::vector<std::string>>(),
                                       state[1].cast<std::string>(),
                                       state[2].cast<std::vector<std::size_t>>(),
                                       state[3].cast<std::size_t>(), state[4].cast<double>(),
                                       state[5].cast<double>());
                    }));

    BindPrimitiveNoBase<GaRfd>(rfd_module, "GaRfd").def("get_rfds", &GaRfd::GetRfds);
}

}  // namespace python_bindings
