#include "bind_md_verification.h"

#include <pybind11/stl.h>

#include "algorithms/md/md_verifier/highlights/highlights.h"
#include "algorithms/md/md_verifier/md_verifier.h"
#include "algorithms/md/md_verifier/md_verifier_column_match.h"
#include "algorithms/md/md_verifier/similarities/euclidean/euclidean.h"
#include "algorithms/md/md_verifier/similarities/jaccard/jaccard.h"
#include "algorithms/md/md_verifier/similarities/levenshtein/levenshtein.h"
#include "algorithms/md/md_verifier/similarities/monge_elkan/monge_elkan.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
using namespace py::literals;
using namespace algos::md;

void BindHighlights(py::module& md_module) {
    py::class_<MDHighlights::Highlight>(md_module, "Highlight")
            .def_readonly("left_table_row", &MDHighlights::Highlight::left_table_row)
            .def_readonly("right_table_row", &MDHighlights::Highlight::right_table_row)
            .def_readonly("similarity", &MDHighlights::Highlight::similarity)
            .def_readonly("decision_boundary", &MDHighlights::Highlight::decision_boundary)
            .def_property_readonly(
                    "similarity_function",
                    [](MDHighlights::Highlight const& record) { return record.column_match.name; })
            .def("ToString", &MDHighlights::Highlight::ToString);
}

template <typename MeasureType>
auto BindSimilarityMeasure(py::module_& measures_module, auto name) {
    auto cls = py::class_<MeasureType, SimilarityMeasure, std::shared_ptr<MeasureType>>(
                       measures_module, name)
                       .def(py::init<>())
                       .def("__call__", &MeasureType::operator());
    return cls;
}

auto BindColumnMatch(py::module_& column_match_module) {
    auto cls =
            py::class_<MDVerifierColumnMatch>(column_match_module, "ColumnMatch")
                    .def(py::init<model::Index, model::Index, std::shared_ptr<SimilarityMeasure>>(),
                         "left_col_index"_a, "right_col_index"_a, "similarity_measure"_a);
}

auto BindColumnSimilarityClassifier(py::module_& column_similarity_classifier_module) {
    auto cls =
            py::class_<MDVerifierColumnSimilarityClassifier>(column_similarity_classifier_module,
                                                             "ColumnSimilarityClassifier")
                    .def(py::init([](algos::md::MDVerifierColumnMatch column_match,
                                     model::md::DecisionBoundary decision_boundary) {
                             return MDVerifierColumnSimilarityClassifier(column_match,
                                                                         decision_boundary);
                         }),
                         "column_match"_a, "decision_boundary"_a)
                    .def(py::init([](model::Index left_col_index, model::Index right_col_index,
                                     std::shared_ptr<algos::md::SimilarityMeasure> measure,
                                     model::md::DecisionBoundary decision_boundary) {
                             return MDVerifierColumnSimilarityClassifier(
                                     left_col_index, right_col_index, measure, decision_boundary);
                         }),
                         "left_col_index"_a, "right_col_index"_a, "similarity_measure"_a,
                         "decision_boundary"_a);
}

void BindMDVerification(py::module_& main_module) {
    auto md_module = main_module.def_submodule("md_verification");

    BindHighlights(md_module);

    auto measures_module = md_module.def_submodule("similarity_measure");
    py::class_<SimilarityMeasure, std::shared_ptr<SimilarityMeasure>>(measures_module,
                                                                      "SimilarityMeasure");
    BindSimilarityMeasure<EuclideanSimilarity>(measures_module, "EuclideanSimilarity");
    BindSimilarityMeasure<LevenshteinSimilarity>(measures_module, "LevenshteinSimilarity");
    BindSimilarityMeasure<MongeElkanSimilarity>(measures_module, "MongeElkanSimilarity");
    BindSimilarityMeasure<JaccardSimilarity>(measures_module, "JaccardSimilarity");

    BindColumnMatch(md_module);
    BindColumnSimilarityClassifier(md_module);

    BindPrimitiveNoBase<MDVerifier>(md_module, "MDVerifier")
            .def("get_highlights", &MDVerifier::GetHighlights)
            .def("get_rhs_suggestions", &MDVerifier::GetRHSSuggestion)
            .def("md_holds", &MDVerifier::GetResult);
}
}  // namespace python_bindings
