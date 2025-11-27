#include "python_bindings/md/bind_md_verification.h"

#include <pybind11/stl.h>

#include "core/algorithms/md/hymd/preprocessing/column_matches/column_match.h"
#include "core/algorithms/md/md_verifier/column_similarity_classifier.h"
#include "core/algorithms/md/md_verifier/highlights/highlights.h"
#include "core/algorithms/md/md_verifier/md_verifier.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
using namespace py::literals;
using namespace algos::md;
using namespace algos::hymd::preprocessing;

void BindHighlights(py::module& md_module) {
    py::class_<MDHighlights::Highlight>(md_module, "Highlight")
            .def_readonly("left_record_id", &MDHighlights::Highlight::left_record_id)
            .def_readonly("right_record_id", &MDHighlights::Highlight::right_record_id)
            .def_readonly("similarity", &MDHighlights::Highlight::similarity)
            .def_property_readonly(
                    "rhs_desc",
                    [](MDHighlights::Highlight const& record) { return record.rhs_desc; })
            .def("to_string", &MDHighlights::Highlight::ToString)
            .def("__str__", &MDHighlights::Highlight::ToString)
            .def("to_string_indexed", &MDHighlights::Highlight::ToStringIndexes);
}

auto BindColumnSimilarityClassifier(py::module_& md_module) {
    auto cls = py::class_<ColumnSimilarityClassifier>(md_module, "ColumnSimilarityClassifier")
                       .def(py::init([](std::shared_ptr<column_matches::ColumnMatch> column_match,
                                        model::md::DecisionBoundary decision_boundary) {
                                return ColumnSimilarityClassifier(column_match, decision_boundary);
                            }),
                            "column_match"_a, "decision_boundary"_a);
}

void BindMDVerification(py::module_& main_module) {
    auto md_module = main_module.def_submodule("md");
    auto md_verification_module = main_module.def_submodule("md_verification");

    BindHighlights(md_module);
    BindColumnSimilarityClassifier(md_module);

    BindPrimitiveNoBase<MDVerifier>(md_verification_module, "MDVerifier")
            .def("get_highlights", &MDVerifier::GetHighlights)
            .def("get_highlights_copy", &MDVerifier::GetHighlightsCopy)
            .def("get_true_rhs_decision_boundary", &MDVerifier::GetTrueRhsDecisionBoundary)
            .def("md_holds", &MDVerifier::GetResult)
            .def("get_md_suggestion", &MDVerifier::GetMDSuggestion)
            .def("get_input_md", &MDVerifier::GetInputMD);
}
}  // namespace python_bindings
