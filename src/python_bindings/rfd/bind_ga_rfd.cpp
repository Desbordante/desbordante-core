#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/algorithms/rfd/similarity_metric.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/rfd/py_similarity_metric.h"

namespace py = pybind11;
using namespace algos::rfd;

namespace python_bindings {

void BindGaRfd(py::module_& m) {
    py::class_<SimilarityMetric, std::shared_ptr<SimilarityMetric>>(m, "SimilarityMetric")
        .def("__call__", &SimilarityMetric::Compare);

    m.def("levenshtein_metric", &LevenshteinMetric);
    m.def("equality_metric", &EqualityMetric);

    BindPrimitiveNoBase<GaRfd>(m, "GaRfd")
        .def("set_similarity_thresholds", &GaRfd::SetSimilarityThresholds)
        .def("set_min_confidence", &GaRfd::SetMinConfidence)
        .def("set_population_size", &GaRfd::SetPopulationSize)
        .def("set_max_generations", &GaRfd::SetMaxGenerations)
        .def("set_crossover_prob", &GaRfd::SetCrossoverProb)
        .def("set_mutation_prob", &GaRfd::SetMutationProb)
        .def("set_max_lhs_arity", &GaRfd::SetMaxLhsArity)
        .def("set_seed", &GaRfd::SetSeed)
        .def("set_output_file", &GaRfd::SetOutputFile)
        .def("set_metrics", &GaRfd::SetMetrics)
        .def("get_rfds", &GaRfd::GetResultStrings)
        .def("save_results", &GaRfd::SaveResults);
}

}  // namespace python_bindings