#include "bind_md_verifier.h"

#include "algorithms/md/md_verifier/md_verifier.h"
#include "algorithms/md/md_verifier/similarities/euclidean/euclidean.h"
#include "algorithms/md/md_verifier/similarities/levenshtein/levenshtein.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {

using SimilarityMeasure = algos::md::SimilarityMeasure;
using EuclideanSimilarity = algos::md::EuclideanSimilarity;
using LevenshteinSimilarity = algos::md::LevenshteinSimilarity;

template <typename MeasureType>
auto BindMeasure(py::module_& measures_module, auto name) {
    auto cls = py::class_<MeasureType, SimilarityMeasure, std::shared_ptr<MeasureType>>(
                       measures_module, name)
                       .def(py::init<>())
                       .def("__call__", &MeasureType::operator());
    return cls;
}

void BindMDVerifier(py::module_& main_module) {
    using namespace algos::md;

    auto md_module = main_module.def_submodule("md_verifier");

    auto measures_module = md_module.def_submodule("similarity_measures");
    py::class_<SimilarityMeasure, std::shared_ptr<SimilarityMeasure>>(measures_module,
                                                                      "SimilarityMeasure");
    BindMeasure<EuclideanSimilarity>(measures_module, "EuclideanSimilarity");
    BindMeasure<LevenshteinSimilarity>(measures_module, "LevenshteinSimilarity");
    BindPrimitiveNoBase<MDVerifier>(md_module, "MDVerifier")
            .def("get_highlights", &MDVerifier::GetHighlights)
            .def("md_holds", &MDVerifier::GetResult);
}
}  // namespace python_bindings
