#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include "core/algorithms/rfd/distance_metric.h"
#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/util/custom_metric/custom_metric.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace py = pybind11;
using algos::rfd::GaRfd;
using algos::rfd::RFD;
using algos::rfd::RFDHash;

namespace python_bindings {

void BindGaRfd(py::module_& main_module) {
    auto rfd_module = main_module.def_submodule("rfd");

    // User-facing similarity facade over core distances: both names refer to
    // the same metric type; Python callables are wrapped similarity -> distance.
    rfd_module.attr("SimilarityMetric") = main_module.attr("metrics").attr("CustomMetric");
    rfd_module.attr("DistanceMetric") = rfd_module.attr("SimilarityMetric");

    rfd_module.def("levenshtein_metric", &algos::rfd::LevenshteinMetric);
    rfd_module.def("equality_metric", &algos::rfd::EqualityMetric);
    rfd_module.def("abs_diff_metric", &algos::rfd::AbsoluteDifferenceMetric);
    rfd_module.def("abs_threshold_metric", &algos::rfd::AbsoluteThresholdMetricFactory,
                   py::arg("tolerance"));

    py::class_<RFD>(rfd_module, "RFD")
            .def(py::init<>())
            .def(py::init([](uint32_t lhs_mask, uint8_t rhs_index, double support,
                             double confidence) {
                     if (rhs_index > algos::rfd::kMaxAttributes - 1)
                         throw std::out_of_range("RHS index must be in [0, 30]");
                     if (lhs_mask & ~((1u << algos::rfd::kMaxAttributes) - 1))
                         throw std::out_of_range("LHS mask must not use attributes above 30");
                     if (lhs_mask & (1u << rhs_index))
                         throw std::invalid_argument("RHS attribute cannot be in LHS");
                     return RFD{lhs_mask, rhs_index, support, confidence};
                 }),
                 py::arg("lhs_mask"), py::arg("rhs_index"), py::arg("support") = 0.0,
                 py::arg("confidence") = 0.0)
            .def_static(
                    "from_lhs_rhs",
                    [](std::vector<int> lhs_indices, int rhs_index, double support,
                       double confidence) {
                        uint32_t mask = 0;
                        for (int idx : lhs_indices) {
                            if (idx < 0 || idx > 30)
                                throw std::out_of_range("Index must be in [0, 30]");
                            mask |= (1u << idx);
                        }
                        if (rhs_index < 0 || rhs_index > 30)
                            throw std::out_of_range("RHS index must be in [0, 30]");
                        if (mask & (1u << rhs_index))
                            throw std::invalid_argument("RHS attribute cannot be in LHS");
                        return RFD{mask, static_cast<uint8_t>(rhs_index), support, confidence};
                    },
                    py::arg("lhs_indices"), py::arg("rhs_index"), py::arg("support") = 0.0,
                    py::arg("confidence") = 0.0)
            .def_property_readonly("lhs_mask", [](RFD const& r) { return r.lhs_mask; })
            .def_property_readonly("rhs_index", [](RFD const& r) { return r.rhs_index; })
            .def_property_readonly("support", [](RFD const& r) { return r.support; })
            .def_property_readonly("confidence", [](RFD const& r) { return r.confidence; })
            .def_property_readonly("lhs",
                                   [](RFD const& r) {
                                       std::vector<int> indices;
                                       for (std::size_t i = 0; i <= algos::rfd::kMaxAttributes;
                                            ++i) {
                                           if (r.lhs_mask & (1u << i))
                                               indices.push_back(static_cast<int>(i));
                                       }
                                       return indices;
                                   })
            .def("__str__", &RFD::ToString)
            .def("__repr__",
                 [](RFD const& r) {
                     return "RFD(lhs_mask=" + std::to_string(r.lhs_mask) +
                            ", rhs_index=" + std::to_string(r.rhs_index) +
                            ", support=" + std::to_string(r.support) +
                            ", confidence=" + std::to_string(r.confidence) + ")";
                 })
            .def("__eq__", [](RFD const& a, RFD const& b) { return a == b; })
            .def("__ne__", [](RFD const& a, RFD const& b) { return a != b; })
            .def("__lt__", [](RFD const& a, RFD const& b) { return a < b; })
            .def("__le__", [](RFD const& a, RFD const& b) { return a <= b; })
            .def("__gt__", [](RFD const& a, RFD const& b) { return a > b; })
            .def("__ge__", [](RFD const& a, RFD const& b) { return a >= b; })
            .def("__hash__", [](RFD const& r) { return RFDHash{}(r); })
            .def(py::pickle(
                    [](RFD const& rfd) {
                        return py::make_tuple(rfd.lhs_mask, rfd.rhs_index, rfd.support,
                                              rfd.confidence);
                    },
                    [](py::tuple state) {
                        if (state.size() != 4) {
                            throw std::runtime_error("Invalid state for RFD pickle");
                        }
                        return RFD{state[0].cast<uint32_t>(), state[1].cast<uint8_t>(),
                                   state[2].cast<double>(), state[3].cast<double>()};
                    }));

    auto ga_cls = BindPrimitiveNoBase<GaRfd>(rfd_module, "GaRfd");

    ga_cls.def("get_rfds", &GaRfd::GetRfds);
}

}  // namespace python_bindings
