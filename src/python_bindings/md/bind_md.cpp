#include "md/bind_md.h"

#include <pybind11/cast.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/typing.h>

#include "algorithms/md/hymd/preprocessing/similarity_measure/date_dif_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/jaccard_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/lcs_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/number_dif_similarity_measure.h"
#include "algorithms/md/md.h"
#include "algorithms/md/mining_algorithms.h"
#include "md/object_similarity_measure.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
using model::MD;
using namespace algos;
using namespace algos::hymd;
using namespace algos::hymd::preprocessing;
using namespace algos::hymd::preprocessing::similarity_measure;
using namespace py::literals;
using namespace python_bindings;

std::vector<ColumnClassifierValueId> PickAll(std::vector<Similarity> const& similarities) {
    return ccv_id_pickers::IndexUniform<Similarity>{0}(similarities);
}

template <typename MeasureType>
auto BindMeasure(auto&& name, auto&& measures_module) {
    auto cls = py::class_<MeasureType, SimilarityMeasure, std::shared_ptr<MeasureType>>(
            measures_module, name);
    return cls;
}

template <typename MeasureType, typename... Args>
void BindMeasureWithConstructor(Args&&... args) {
    using ColumnFunctions = MeasureType::TransformFunctionsOption;
    BindMeasure<MeasureType>(std::forward<Args>(args)...)
            .def(py::init<ColumnIdentifier, ColumnIdentifier, model::md::DecisionBoundary,
                          std::size_t, ColumnFunctions>(),
                 "left_column"_a, "right_column"_a, "minimum_similarity"_a = 0.7, py::kw_only(),
                 "bound_number_limit"_a = 0, "column_functions"_a = ColumnFunctions{})
            .def(py::init([](ColumnIdentifier left_column, ColumnIdentifier right_column,
                             model::md::DecisionBoundary min_sim,
                             py::typing::Callable<std::vector<ColumnClassifierValueId>(
                                     std::vector<Similarity> const&)>
                                     similarities_picker,
                             ColumnFunctions column_functions) {
                     return std::make_shared<MeasureType>(
                             std::move(left_column), std::move(right_column), min_sim,
                             PyLhsCCVIDsPicker(std::move(similarities_picker)),
                             std::move(column_functions));
                 }),
                 "left_column"_a, "right_column"_a, "minimum_similarity"_a = 0.7, py::kw_only(),
                 py::arg_v("pick_lhs_indices", ccv_id_pickers::SimilaritiesPicker(PickAll),
                           "pick_all")
                         .none(false),
                 "column_functions"_a = ColumnFunctions{});
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
            .def("to_string_active", &MD::ToStringActiveLhsOnly)
            .def("__str__", &MD::ToStringShort);
    auto measures_module = md_module.def_submodule("similarity_measures");
    py::class_<SimilarityMeasure, std::shared_ptr<SimilarityMeasure>>(measures_module,
                                                                      "SimilarityMeasure");
    BindMeasureWithConstructor<LevenshteinSimilarityMeasure>("LevenshteinSimilarity",
                                                             measures_module);
    BindMeasureWithConstructor<MongeElkanSimilarityMeasure>("MongeElkanSimilarity",
                                                            measures_module);
    BindMeasureWithConstructor<JaccardSimilarityMeasure>("JaccardSimilarity", measures_module);
    BindMeasureWithConstructor<DateSimilarityMeasure>("DateSimilarity", measures_module);
    BindMeasureWithConstructor<NumberSimilarityMeasure>("NumberSimilarity", measures_module);
    BindMeasureWithConstructor<LcsSimilarityMeasure>("LcsSimilarity", measures_module);

    BindMeasure<ObjectSimilarityMeasure>("ObjectSimilarityMeasure", measures_module)
            .def(py::init<py::typing::Callable<preprocessing::Similarity(py::object, py::object)>,
                          ColumnIdentifier, ColumnIdentifier, ObjMeasureTransformFuncs, bool, bool,
                          model::md::DecisionBoundary, std::string, std::size_t>(),
                 "comparer"_a, "left_column"_a, "right_column"_a, py::kw_only(),
                 "column_functions"_a = ObjMeasureTransformFuncs{}, "symmetrical"_a = false,
                 "equality_is_max"_a = false, "min_sim"_a = 0.7,
                 "measure_name"_a = "custom_measure", "size_limit"_a = 0)
            .def(py::init<py::object, ColumnIdentifier, ColumnIdentifier, bool,
                          ObjMeasureTransformFuncs, model::md::DecisionBoundary, std::string,
                          std::size_t>(),
                 "comparer"_a, "left_column"_a, "right_column"_a, "classic_measure"_a,
                 py::kw_only(), "column_functions"_a = ObjMeasureTransformFuncs{},
                 "min_sim"_a = 0.7, "measure_name"_a = "custom_measure", "size_limit"_a = 0)
            .def(py::init<py::typing::Callable<preprocessing::Similarity(py::object, py::object)>,
                          ColumnIdentifier, ColumnIdentifier, ObjMeasureTransformFuncs, bool, bool,
                          model::md::DecisionBoundary, std::string,
                          py::typing::Callable<std::vector<ColumnClassifierValueId>(
                                  std::vector<Similarity> const&)>>(),
                 "comparer"_a, "left_column"_a, "right_column"_a, py::kw_only(),
                 "column_functions"_a = ObjMeasureTransformFuncs{}, "symmetrical"_a = false,
                 "equality_is_max"_a = false, "min_sim"_a = 0.7,
                 "measure_name"_a = "custom_measure",
                 py::arg_v("pick_lhs_indices", ccv_id_pickers::SimilaritiesPicker(PickAll),
                           "pick_all")
                         .none(false))
            .doc() = R"(Defines a custom similarity measure.)";

    BindPrimitive<HyMD>(md_module, &MdAlgorithm::MdList, "MdAlgorithm", "get_mds", {"HyMD"},
                        pybind11::return_value_policy::copy);
}
}  // namespace python_bindings
