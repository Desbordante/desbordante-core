#include "md/bind_md.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/md/hymd/preprocessing/similarity_measure/date_dif_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/jaccard_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/lcs_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/number_dif_similarity_measure.h"
#include "algorithms/md/hymd/similarity_measure_creator.h"
#include "algorithms/md/md.h"
#include "algorithms/md/mining_algorithms.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
using model::MD;
}  // namespace

namespace python_bindings {
void BindMd(py::module_& main_module) {
    using namespace algos;
    using namespace algos::hymd;
    using namespace algos::hymd::preprocessing::similarity_measure;
    using namespace py::literals;

    auto md_module = main_module.def_submodule("md");
    py::class_<MD>(md_module, "MD")
            .def_property_readonly("lhs_bounds", &MD::GetLhsDecisionBounds)
            .def_property_readonly("rhs", &MD::GetRhs)
            .def("to_long_string", &MD::ToStringFull)
            .def("to_short_string", &MD::ToStringShort)
            .def("__str__", &MD::ToStringShort);
    auto measures_module = md_module.def_submodule("similarity_measures");
    py::class_<SimilarityMeasureCreator, std::shared_ptr<SimilarityMeasureCreator>>(
            measures_module, "SimilarityMeasure");
    py::class_<LevenshteinSimilarityMeasure::Creator, SimilarityMeasureCreator,
               std::shared_ptr<LevenshteinSimilarityMeasure::Creator>>(measures_module,
                                                                       "LevenshteinSimilarity")
            .def(py::init<ColumnIdentifier, ColumnIdentifier, model::md::DecisionBoundary,
                          std::size_t>(),
                 "left_column"_a, "right_column"_a, "minimum_similarity"_a = 0.7,
                 "bound_number_limit"_a = 0);
    py::class_<MongeElkanSimilarityMeasure::Creator, SimilarityMeasureCreator,
               std::shared_ptr<MongeElkanSimilarityMeasure::Creator>>(measures_module,
                                                                      "MongeElkanSimilarity")
            .def(py::init<ColumnIdentifier, ColumnIdentifier, model::md::DecisionBoundary,
                          std::size_t>(),
                 "left_column"_a, "right_column"_a, "minimum_similarity"_a = 0.7,
                 "bound_number_limit"_a = 0);

    py::class_<JaccardSimilarityMeasure::Creator, SimilarityMeasureCreator,
               std::shared_ptr<JaccardSimilarityMeasure::Creator>>(measures_module,
                                                                   "JaccardSimilarity")
            .def(py::init<ColumnIdentifier, ColumnIdentifier, model::md::DecisionBoundary,
                          std::size_t>(),
                 "left_column"_a, "right_column"_a, "minimum_similarity"_a = 0.7,
                 "bound_number_limit"_a = 0);

    py::class_<DateSimilarityMeasure::Creator, SimilarityMeasureCreator,
               std::shared_ptr<DateSimilarityMeasure::Creator>>(measures_module, "DateSimilarity")
            .def(py::init<ColumnIdentifier, ColumnIdentifier, model::md::DecisionBoundary,
                          std::size_t>(),
                 "left_column"_a, "right_column"_a, "minimum_similarity"_a = 0.7,
                 "bound_number_limit"_a = 0);

    py::class_<NumberSimilarityMeasure::Creator, SimilarityMeasureCreator,
               std::shared_ptr<NumberSimilarityMeasure::Creator>>(measures_module,
                                                                  "NumberSimilarity")
            .def(py::init<ColumnIdentifier, ColumnIdentifier, model::md::DecisionBoundary,
                          std::size_t>(),
                 "left_column"_a, "right_column"_a, "minimum_similarity"_a = 0.7,
                 "bound_number_limit"_a = 0);

    py::class_<LcsSimilarityMeasure::Creator, SimilarityMeasureCreator,
               std::shared_ptr<LcsSimilarityMeasure::Creator>>(measures_module, "LcsSimilarity")
            .def(py::init<ColumnIdentifier, ColumnIdentifier, model::md::DecisionBoundary,
                          std::size_t>(),
                 "left_column"_a, "right_column"_a, "minimum_similarity"_a = 0.7,
                 "bound_number_limit"_a = 0);

    BindPrimitive<HyMD>(md_module, &MdAlgorithm::MdList, "MdAlgorithm", "get_mds", {"HyMD"});
}
}  // namespace python_bindings
