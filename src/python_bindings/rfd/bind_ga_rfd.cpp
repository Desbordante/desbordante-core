#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/algorithms/rfd/similarity_metric.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/rfd/py_similarity_metric.h"

namespace py = pybind11;
using algos::rfd::GaRfd;
using algos::rfd::RFD;
using algos::rfd::SimilarityMetric;

namespace python_bindings {

std::vector<std::shared_ptr<SimilarityMetric>> ConvertMetrics(const std::vector<py::object>& objs) {
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

    py::class_<RFD>(rfd_module, "RFD")
        .def(py::init<>())
        .def_property_readonly("lhs_mask", [](RFD const& r) { return r.lhs_mask; })
        .def_property_readonly("rhs_index", [](RFD const& r) { return r.rhs_index; })
        .def_property_readonly("support", [](RFD const& r) { return r.support; })
        .def_property_readonly("confidence", [](RFD const& r) { return r.confidence; })
        .def("__str__", &RFD::ToString)
        .def("__repr__", &RFD::ToString);

    auto ga_cls = BindPrimitiveNoBase<GaRfd>(rfd_module, "GaRfd");

    ga_cls.def("set_metrics", &GaRfd::SetMetrics, "Set list of SimilarityMetric objects");

    ga_cls.def("set_metrics_py", [](GaRfd& self, const std::vector<py::object>& metrics) {
        self.SetMetrics(ConvertMetrics(metrics));
    }, "Set list of metrics (Python functions or SimilarityMetric objects)");

    ga_cls.def("get_rfds", &GaRfd::GetRfds);
}

}