#include "md/bind_md.h"

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/md/hymd/preprocessing/similarity_measure/date_dif_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/jaccard_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/lcs_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/number_dif_similarity_measure.h"
#include "algorithms/md/md.h"
#include "algorithms/md/mining_algorithms.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
using model::MD;
using namespace algos;
using namespace algos::hymd;
using namespace algos::hymd::preprocessing::similarity_measure;
using namespace py::literals;

template <typename MeasureType>
void BindMeasure(auto&& name, auto&& measures_module) {
    py::class_<MeasureType, SimilarityMeasure, std::shared_ptr<MeasureType>>(measures_module, name)
            .def(py::init<ColumnIdentifier, ColumnIdentifier, model::md::DecisionBoundary,
                          std::size_t, typename MeasureType::TransformFunctionsOption>(),
                 "left_column"_a, "right_column"_a, "minimum_similarity"_a = 0.7, py::kw_only(),
                 "bound_number_limit"_a = 0,
                 "column_functions"_a = typename MeasureType::TransformFunctionsOption{});
}
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
    py::class_<SimilarityMeasure, std::shared_ptr<SimilarityMeasure>>(measures_module,
                                                                      "SimilarityMeasure");
    BindMeasure<LevenshteinSimilarityMeasure>("LevenshteinSimilarity", measures_module);
    BindMeasure<MongeElkanSimilarityMeasure>("MongeElkanSimilarity", measures_module);
    BindMeasure<JaccardSimilarityMeasure>("JaccardSimilarity", measures_module);
    BindMeasure<DateSimilarityMeasure>("DateSimilarity", measures_module);
    BindMeasure<NumberSimilarityMeasure>("NumberSimilarity", measures_module);
    BindMeasure<LcsSimilarityMeasure>("LcsSimilarity", measures_module);

    BindPrimitive<HyMD>(md_module, &MdAlgorithm::MdList, "MdAlgorithm", "get_mds", {"HyMD"});
}
}  // namespace python_bindings
