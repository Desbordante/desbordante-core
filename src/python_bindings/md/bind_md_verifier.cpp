#include "bind_md_verifier.h"

#include <pybind11/stl.h>

#include "algorithms/md/md_verifier/highlights/highlights.h"
#include "algorithms/md/md_verifier/md_verifier.h"
#include "algorithms/md/md_verifier/similarities/euclidean/euclidean.h"
#include "algorithms/md/md_verifier/similarities/levenshtein/levenshtein.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {

using namespace algos::md;

void BindHighlights(py::module& md_module) {
    using HighlightRecord = MDHighlights::HighlightRecord;
    py::class_<HighlightRecord>(md_module, "Highlight")
            .def(py::init<>())
            .def_property_readonly("rows",
                                   [](HighlightRecord const& record) {
                                       return py::make_tuple(record.rows.first, record.rows.second);
                                   })
            .def_readonly("column", &HighlightRecord::column)
            .def_readonly("first_value", &HighlightRecord::first_value)
            .def_readonly("second_value", &HighlightRecord::second_value)
            .def_readonly("similarity", &HighlightRecord::similarity)
            .def_readonly("decision_boundary", &HighlightRecord::decision_boundary);
}

template <typename MeasureType>
auto BindMeasure(py::module_& measures_module, auto name) {
    auto cls = py::class_<MeasureType, SimilarityMeasure, std::shared_ptr<MeasureType>>(
                       measures_module, name)
                       .def(py::init<>())
                       .def("__call__", &MeasureType::operator());
    return cls;
}

void BindMDVerifier(py::module_& main_module) {
    auto md_module = main_module.def_submodule("md_verifier");

    BindHighlights(md_module);

    auto measures_module = md_module.def_submodule("similarity_measures");
    py::class_<SimilarityMeasure, std::shared_ptr<SimilarityMeasure>>(measures_module,
                                                                      "SimilarityMeasure");
    BindMeasure<EuclideanSimilarity>(measures_module, "EuclideanSimilarity");
    BindMeasure<LevenshteinSimilarity>(measures_module, "LevenshteinSimilarity");
    BindPrimitiveNoBase<MDVerifier>(md_module, "MDVerifier")
            .def("get_highlights", &MDVerifier::GetHighlights)
            .def("get_highlights_as_strings", &MDVerifier::GetHighlightsAsStrings)
            .def("get_rhs_suggestions", &MDVerifier::GetRhsSuggestions)
            .def("md_holds", &MDVerifier::GetResult);
}
}  // namespace python_bindings
