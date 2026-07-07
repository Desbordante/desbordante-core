#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/algorithms/rfd/similarity_metric.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/rfd/py_similarity_metric.h"

namespace py = pybind11;
using algos::rfd::GaRfd;
using algos::rfd::RFD;
using algos::rfd::RFDHash;
using algos::rfd::SimilarityMetric;

namespace python_bindings {

std::vector<std::shared_ptr<SimilarityMetric>> ConvertMetrics(std::vector<py::object> const& objs) {
    std::vector<std::shared_ptr<SimilarityMetric>> metrics;
    metrics.reserve(objs.size());
    for (py::object obj : objs) {
        if (py::isinstance<py::function>(obj)) {
            metrics.push_back(std::make_shared<PySimilarityMetric>(std::move(obj)));
        } else {
            metrics.push_back(obj.cast<std::shared_ptr<SimilarityMetric>>());
        }
    }
    return metrics;
}

void BindGaRfd(py::module_& main_module) {
    auto rfd_module = main_module.def_submodule("rfd");

    py::class_<SimilarityMetric, std::shared_ptr<SimilarityMetric>>(rfd_module, "SimilarityMetric")
            .def("__call__", &SimilarityMetric::Compare);

    rfd_module.def("levenshtein_metric", &algos::rfd::LevenshteinMetric);
    rfd_module.def("equality_metric", &algos::rfd::EqualityMetric);
    rfd_module.def("abs_diff_metric", &algos::rfd::AbsoluteDifferenceMetric);
    rfd_module.def("abs_threshold_metric", &algos::rfd::AbsoluteThresholdMetric);

    py::class_<RFD>(rfd_module, "RFD")
            .def(py::init<>())
            .def(py::init<uint32_t, uint8_t, double, double>(), py::arg("lhs_mask"),
                 py::arg("rhs_index"), py::arg("support") = 0.0, py::arg("confidence") = 0.0)
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
                                       for (int i = 0; i < 31; ++i) {
                                           if (r.lhs_mask & (1u << i)) indices.push_back(i);
                                       }
                                       return indices;
                                   })
            .def("__str__", &RFD::ToString)
            .def("__repr__", &RFD::ToString)
            .def("__eq__", [](RFD const& a, RFD const& b) { return a == b; })
            .def("__ne__", [](RFD const& a, RFD const& b) { return a != b; })
            .def("__lt__", [](RFD const& a, RFD const& b) { return a < b; })
            .def("__le__", [](RFD const& a, RFD const& b) { return a <= b; })
            .def("__gt__", [](RFD const& a, RFD const& b) { return a > b; })
            .def("__ge__", [](RFD const& a, RFD const& b) { return a >= b; })
            .def("__hash__", [](RFD const& r) { return RFDHash{}(r); })
            .def("__getstate__",
                 [](RFD const& r) {
                     py::dict d;
                     d["lhs_mask"] = r.lhs_mask;
                     d["rhs_index"] = r.rhs_index;
                     d["support"] = r.support;
                     d["confidence"] = r.confidence;
                     return d;
                 })
            .def("__setstate__", [](RFD& r, py::dict state) {
                r.lhs_mask = state["lhs_mask"].cast<uint32_t>();
                r.rhs_index = state["rhs_index"].cast<uint8_t>();
                r.support = state["support"].cast<double>();
                r.confidence = state["confidence"].cast<double>();
            });

    auto ga_cls = BindPrimitiveNoBase<GaRfd>(rfd_module, "GaRfd");

    ga_cls.def(
            "set_metrics",
            [](GaRfd& self, std::vector<py::object> const& metrics) {
                self.SetMetrics(ConvertMetrics(metrics));
            },
            "Set list of similarity metrics (built-in metric objects or Python callables)");

    ga_cls.def("get_rfds", &GaRfd::GetRfds);
}

}  // namespace python_bindings
