#include "python_bindings/md/bind_md.h"

#include <pybind11/cast.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/typing.h>

#include "core/algorithms/md/hymd/preprocessing/column_matches/date_difference.h"
#include "core/algorithms/md/hymd/preprocessing/column_matches/equality.h"
#include "core/algorithms/md/hymd/preprocessing/column_matches/jaccard.h"
#include "core/algorithms/md/hymd/preprocessing/column_matches/lcs.h"
#include "core/algorithms/md/hymd/preprocessing/column_matches/levenshtein.h"
#include "core/algorithms/md/hymd/preprocessing/column_matches/monge_elkan.h"
#include "core/algorithms/md/hymd/preprocessing/column_matches/number_difference.h"
#include "core/algorithms/md/md.h"
#include "core/algorithms/md/mining_algorithms.h"
#include "python_bindings/md/object_similarity_measure.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/table_serialization.h"

namespace {
namespace py = pybind11;
using model::MD;
using namespace algos;
using namespace algos::hymd;
using namespace algos::hymd::preprocessing;
using namespace algos::hymd::preprocessing::column_matches;
using namespace py::literals;
using namespace python_bindings;

std::vector<ColumnClassifierValueId> PickAll(std::vector<Similarity> const& similarities) {
    return ccv_id_pickers::IndexUniform<Similarity>{0}(similarities);
}

template <typename MeasureType>
auto BindColumnMatch(auto&& name, auto&& measures_module) {
    auto cls = py::class_<MeasureType, ColumnMatch, std::shared_ptr<MeasureType>>(measures_module,
                                                                                  name);
    return cls;
}

template <typename MeasureType, typename... Args>
void BindColumnMatchWithConstructor(Args&&... args) {
    using ColumnFunctions = MeasureType::TransformFunctionsOption;
    BindColumnMatch<MeasureType>(std::forward<Args>(args)...)
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

py::tuple SerializeColumnMatch(MD const& md_obj) {
    std::vector<py::tuple> match_tuples;
    std::shared_ptr<std::vector<model::md::ColumnMatch> const> matches_ptr = md_obj.GetColumnMatches();
    if (matches_ptr) {
        for (model::md::ColumnMatch const& cm : *matches_ptr) {
            match_tuples.push_back(py::make_tuple(cm.left_col_index, cm.right_col_index, cm.name));
        }
    }
    py::tuple match_tuple = py::cast(match_tuples);
    return match_tuple;
}

py::tuple SerializeLhs(MD const& md_obj) {
    std::vector<py::tuple> lhs_tuples;
    for (model::md::LhsColumnSimilarityClassifier const& csc : md_obj.GetLhs()) {
        std::size_t match_idx = csc.GetColumnMatchIndex();
        model::md::DecisionBoundary db = csc.GetDecisionBoundary();
        std::optional<model::md::DecisionBoundary> maybe_max = csc.GetMaxDisprovedBound();
        lhs_tuples.push_back(py::make_tuple(match_idx, db, maybe_max));
    }
    py::tuple lhs_tuple = py::cast(lhs_tuples);
    return lhs_tuple;
}

py::tuple SerializeRhs(MD const& md_obj) {
    auto [rhs_idx, rhs_bound] = md_obj.GetRhs();
    return py::make_tuple(rhs_idx, rhs_bound);
}

py::tuple SerializeMD(MD const& md_obj) {
    py::tuple left_schema_state = table_serialization::SerializeRelationalSchema(
            md_obj.GetLeftSchema().get());
    py::tuple right_schema_state = table_serialization::SerializeRelationalSchema(
            md_obj.GetRightSchema().get());

    py::tuple match_tuple = SerializeColumnMatch(md_obj);

    py::tuple lhs_tuple = SerializeLhs(md_obj);

    py::tuple rhs_tuple = SerializeRhs(md_obj);

    return py::make_tuple(std::move(left_schema_state), std::move(right_schema_state),
                         std::move(match_tuple), std::move(lhs_tuple), std::move(rhs_tuple));
}
}  // namespace

namespace python_bindings {
void BindMd(py::module_& main_module) {
    using namespace algos;
    using namespace algos::hymd;
    using namespace algos::hymd::preprocessing::column_matches;
    using namespace py::literals;
    using namespace model;

    auto md_module = main_module.def_submodule("md");
    py::class_<ColumnDescription>(md_module, "ColumnDescription")
            .def_readonly("column_name", &ColumnDescription::column_name)
            .def_readonly("column_index", &ColumnDescription::column_index);

    py::class_<ColumnMatchDescription>(md_module, "ColumnMatchDescription")
            .def_readonly("left_column_description",
                          &ColumnMatchDescription::left_column_description)
            .def_readonly("right_column_description",
                          &ColumnMatchDescription::right_column_description)
            .def_readonly("column_match_name", &ColumnMatchDescription::column_match_name);

    py::class_<LhsSimilarityClassifierDesctription>(md_module,
                                                    "LhsSimilarityClassifierDesctription")
            .def_readonly("column_match_description",
                          &LhsSimilarityClassifierDesctription::column_match_description)
            .def_readonly("decision_boundary",
                          &LhsSimilarityClassifierDesctription::decision_boundary)
            .def_readonly("max_invalid_bound",
                          &LhsSimilarityClassifierDesctription::max_invalid_bound);

    py::class_<RhsSimilarityClassifierDesctription>(md_module,
                                                    "RhsSimilarityClassifierDesctription")
            .def_readonly("column_match_description",
                          &RhsSimilarityClassifierDesctription::column_match_description)
            .def_readonly("decision_boundary",
                          &RhsSimilarityClassifierDesctription::decision_boundary);
    py::class_<MDDescription>(md_module, "MDDescription")
            .def_readonly("left_table_name", &MDDescription::left_table_name)
            .def_readonly("right_table_name", &MDDescription::right_table_name)
            .def_readonly("lhs", &MDDescription::lhs)
            .def_readonly("rhs", &MDDescription::rhs);

    py::class_<MD>(md_module, "MD")
            .def_property_readonly("lhs_bounds", &MD::GetLhsDecisionBounds)
            .def_property_readonly("rhs", &MD::GetRhs)
            .def("to_long_string", &MD::ToStringFull)
            .def("to_short_string", &MD::ToStringShort)
            .def("to_string_active", &MD::ToStringActiveLhsOnly)
            .def("__str__", &MD::ToStringActiveLhsOnly)
            .def_property_readonly("single_table", &MD::SingleTable)
            .def("get_description", &MD::GetDescription)
            .def("__eq__", [](MD const& md1, MD const& md2){
                return md1.ToStringFull() == md2.ToStringFull();

            })
            .def("__hash__", [](MD const& md_obj){
                py::tuple left_schema_tuple = table_serialization::ConvertSchemaToImmutableTuple(md_obj.GetLeftSchema().get());
                py::tuple right_schema_tuple = table_serialization::ConvertSchemaToImmutableTuple(md_obj.GetRightSchema().get());

                py::tuple match_tuple = SerializeColumnMatch(md_obj);
                py::tuple lhs_tuple = SerializeLhs(md_obj);
                py::tuple rhs_tuple = SerializeRhs(md_obj);

                py::tuple state_tuple = py::make_tuple(std::move(left_schema_tuple), std::move(right_schema_tuple),
                    std::move(match_tuple), std::move(lhs_tuple), std::move(rhs_tuple));
                return py::hash(state_tuple);

                
            })
            .def(py::pickle(
                    // __getstate__
                    [](MD const& md_obj) { return SerializeMD(md_obj); },
                    // __setstate__
                    [](py::tuple st) {
                        if (st.size() != 5) {
                            throw std::runtime_error("Invalid state for MD pickle!");
                        }
                        std::shared_ptr<RelationalSchema const> left_schema =
                                table_serialization::DeserializeRelationalSchema(
                                        st[0].cast<py::tuple>());
                        std::shared_ptr<RelationalSchema const> right_schema =
                                table_serialization::DeserializeRelationalSchema(
                                        st[1].cast<py::tuple>());
                        auto match_tuple = st[2].cast<py::tuple>();
                        auto matches_ptr = std::make_shared<std::vector<model::md::ColumnMatch>>();
                        matches_ptr->reserve(match_tuple.size());
                        for (auto item : match_tuple) {
                            auto tpl = item.cast<py::tuple>();
                            if (tpl.size() != 3) {
                                throw std::runtime_error("Invalid state for MD pickle!");
                            }
                            auto l_idx = tpl[0].cast<std::size_t>();
                            auto r_idx = tpl[1].cast<std::size_t>();
                            auto name = tpl[2].cast<std::string>();
                            matches_ptr->emplace_back(l_idx, r_idx, std::move(name));
                        }
                        auto lhs_tuple = st[3].cast<py::tuple>();
                        std::vector<model::md::LhsColumnSimilarityClassifier> lhs_vec;
                        lhs_vec.reserve(lhs_tuple.size());
                        for (auto item : lhs_tuple) {
                            auto tpl = item.cast<py::tuple>();
                            if (tpl.size() != 3) {
                                throw std::runtime_error("Invalid state for MD pickle!");
                            }
                            auto match_idx = tpl[0].cast<std::size_t>();
                            auto dec_bound = tpl[1].cast<double>();
                            std::optional<model::md::DecisionBoundary> restored_maybe_max =
                                    tpl[2].cast<std::optional<model::md::DecisionBoundary>>();
                            lhs_vec.emplace_back(restored_maybe_max, match_idx, dec_bound);
                        }
                        auto rhs_tpl = st[4].cast<py::tuple>();
                        if (rhs_tpl.size() != 2) {
                            throw std::runtime_error("Invalid state for MD pickle!");
                        }
                        auto rhs_idx = rhs_tpl[0].cast<std::size_t>();
                        auto rhs_dec = rhs_tpl[1].cast<double>();
                        model::md::ColumnSimilarityClassifier rhs_classifier(rhs_idx, rhs_dec);
                        model::MD md_restored(std::move(left_schema), std::move(right_schema),
                                              std::move(matches_ptr), std::move(lhs_vec),
                                              std::move(rhs_classifier));
                        return md_restored;
                    }));

    auto column_matches_module = md_module.def_submodule("column_matches");
    py::class_<ColumnMatch, std::shared_ptr<ColumnMatch>>(column_matches_module, "ColumnMatch");
    BindColumnMatchWithConstructor<Levenshtein>("Levenshtein", column_matches_module);
    BindColumnMatchWithConstructor<MongeElkan>("MongeElkan", column_matches_module);
    BindColumnMatchWithConstructor<Jaccard>("Jaccard", column_matches_module);
    BindColumnMatchWithConstructor<LVNormDateDifference>("LVNormDateDistance",
                                                         column_matches_module);
    BindColumnMatchWithConstructor<LVNormNumberDistance>("LVNormNumberDistance",
                                                         column_matches_module);
    BindColumnMatchWithConstructor<Lcs>("Lcs", column_matches_module);
    BindColumnMatch<Equality>("Equality", column_matches_module)
            .def(py::init<ColumnIdentifier, ColumnIdentifier>(), "left_column"_a, "right_column"_a);

    BindColumnMatch<Custom>("Custom", column_matches_module)
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
                 "equality_is_max"_a = false, "min_sim"_a = 0.7, "measure_name"_a = "custom",
                 py::arg_v("pick_lhs_indices", ccv_id_pickers::SimilaritiesPicker(PickAll),
                           "pick_all")
                         .none(false))
            .doc() = R"(Defines a column match with a custom similarity measure.)";

    BindPrimitive<HyMD>(md_module, &MdAlgorithm::MdList, "MdAlgorithm", "get_mds", {"HyMD"},
                        pybind11::return_value_policy::copy);
}
}  // namespace python_bindings
