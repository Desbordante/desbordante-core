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

void BindGaRfd(py::module_& main_module) {
    auto rfd_module = main_module.def_submodule("rfd");

    py::class_<SimilarityMetric, std::shared_ptr<SimilarityMetric>>(rfd_module, "SimilarityMetric")
        .def("__call__", &SimilarityMetric::Compare);

    rfd_module.def("levenshtein_metric", &algos::rfd::LevenshteinMetric);
    rfd_module.def("equality_metric", &algos::rfd::EqualityMetric);

    py::class_<RFD>(rfd_module, "RFD")
        .def(py::init<>())
        .def_property_readonly("lhs_mask", [](RFD const& r) { return r.lhs_mask; })
        .def_property_readonly("rhs_index", [](RFD const& r) { return r.rhs_index; })
        .def_property_readonly("support", [](RFD const& r) { return r.support; })
        .def_property_readonly("confidence", [](RFD const& r) { return r.confidence; })
        .def("__str__", &RFD::ToString)
        .def("__repr__", &RFD::ToString);

    auto ga_cls = BindPrimitiveNoBase<GaRfd>(rfd_module, "GaRfd");
    ga_cls.def("set_metrics", &GaRfd::SetMetrics)
          .def("get_rfds", &GaRfd::GetRfds);
}

}  // namespace python_bindings
