#include "python_bindings/ind/bind_ind.h"

#include <cmath>

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include "core/algorithms/ind/ind.h"
#include "core/algorithms/ind/ind_algorithm.h"
#include "core/algorithms/ind/mining_algorithms.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/table_serialization.h"

namespace py = pybind11;

namespace {
constexpr double kRoundingValue = 1e12;

}  // namespace

namespace ind_serialization {

py::tuple SerializeInd(model::IND const& ind) {
    py::object lhs_state = py::cast(ind.GetLhs());
    py::object rhs_state = py::cast(ind.GetRhs());

    std::vector<std::unique_ptr<RelationalSchema>> const& schemas_vec = *(ind.GetSchemas());
    std::vector<py::tuple> schemas_state;
    for (auto const& schema : schemas_vec) {
        std::string s_name = schema->GetName();
        std::vector<std::string> s_col_names;
        for (std::unique_ptr<Column> const& col_ptr : schema->GetColumns()) {
            s_col_names.push_back(col_ptr->GetName());
        }
        schemas_state.push_back(py::make_tuple(std::move(s_name), std::move(s_col_names)));
    }
    double error = ind.GetError();
    return py::make_tuple(std::move(lhs_state), std::move(rhs_state), std::move(schemas_state),
                          error);
}

py::tuple ConvertIndToImmutableTuple(model::IND const& ind) {
    auto lhs_schema_index = ind.GetLhs().GetTableIndex();
    auto rhs_schema_index = ind.GetRhs().GetTableIndex();

    auto const& schemas = *(ind.GetSchemas());
    auto const& lhs_schema = schemas[lhs_schema_index];
    auto const& rhs_schema = schemas[rhs_schema_index];

    py::tuple lhs_schema_tuple =
            table_serialization::ConvertSchemaToImmutableTuple(lhs_schema.get());
    py::tuple rhs_schema_tuple =
            table_serialization::ConvertSchemaToImmutableTuple(rhs_schema.get());

    std::vector<unsigned int> lhs_indices = ind.GetLhs().GetColumnIndices();
    py::tuple lhs_tuple = py::tuple(lhs_indices.size());
    for (size_t i = 0; i < lhs_indices.size(); ++i) {
        lhs_tuple[i] = lhs_indices[i];
    }

    std::vector<unsigned int> rhs_indices = ind.GetRhs().GetColumnIndices();
    py::tuple rhs_tuple = py::tuple(rhs_indices.size());
    for (size_t i = 0; i < rhs_indices.size(); ++i) {
        rhs_tuple[i] = rhs_indices[i];
    }

    auto round_double_val = [](double d) {
        return std::round(d * kRoundingValue) / kRoundingValue;
    };

    return py::make_tuple(round_double_val(ind.GetError()), std::move(lhs_schema_tuple),
                          std::move(lhs_tuple), std::move(rhs_schema_tuple), std::move(rhs_tuple));
}
}  // namespace ind_serialization

namespace python_bindings {
void BindInd(py::module_& main_module) {
    using namespace model;
    using namespace algos;

    auto ind_module = main_module.def_submodule("ind");

    py::class_<IND>(ind_module, "IND")
            .def("__str__", &IND::ToLongString)
            .def("to_short_string", &IND::ToShortString)
            .def("to_long_string", &IND::ToLongString)
            .def("get_lhs", &IND::GetLhs)
            .def("get_rhs", &IND::GetRhs)
            .def("get_error", &IND::GetError)
            .def("__eq__",
                 [](IND const& ind1, IND const& ind2) {
                     if (&ind1 == &ind2) {
                         return true;
                     }
                     py::tuple ind1_state_tuple =
                             ind_serialization::ConvertIndToImmutableTuple(ind1);
                     py::tuple ind2_state_tuple =
                             ind_serialization::ConvertIndToImmutableTuple(ind2);
                     return ind1_state_tuple.equal(ind2_state_tuple);
                 })
            .def("__hash__",
                 [](IND const& ind) {
                     py::tuple ind_state_tuple = ind_serialization::ConvertIndToImmutableTuple(ind);
                     return py::hash(ind_state_tuple);
                 })
            .def(py::pickle(
                    // __getstate__
                    [](IND const& ind) { return ind_serialization::SerializeInd(ind); },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 4) {
                            throw std::runtime_error("Invalid state for IND pickle!");
                        }
                        ColumnCombination lhs_cc = t[0].cast<ColumnCombination>();
                        ColumnCombination rhs_cc = t[1].cast<ColumnCombination>();
                        auto lhs_ptr = std::make_shared<ColumnCombination>(lhs_cc);
                        auto rhs_ptr = std::make_shared<ColumnCombination>(rhs_cc);

                        auto schemas_state = t[2].cast<std::vector<py::tuple>>();
                        std::vector<std::unique_ptr<RelationalSchema>> schemas;
                        for (py::tuple const& s_state : schemas_state) {
                            std::string s_name = s_state[0].cast<std::string>();
                            std::vector<std::string> s_col_names =
                                    s_state[1].cast<std::vector<std::string>>();
                            auto schema = std::make_unique<RelationalSchema>(std::move(s_name));
                            for (std::string const& col_name : s_col_names) {
                                schema->AppendColumn(col_name);
                            }
                            schemas.push_back(std::move(schema));
                        }
                        auto schemas_ptr =
                                std::make_shared<std::vector<std::unique_ptr<RelationalSchema>>>(
                                        std::move(schemas));
                        double error = t[3].cast<double>();
                        return IND(std::move(lhs_ptr), std::move(rhs_ptr), std::move(schemas_ptr),
                                   error);
                    }));

    static constexpr auto kSpiderName = "Spider";
    static constexpr auto kMindName = "Mind";
    static constexpr auto kFaidaName = "Faida";

    auto ind_algos_module =
            BindPrimitive<Spider, Faida, Mind>(ind_module, &INDAlgorithm::INDList, "IndAlgorithm",
                                               "get_inds", {kSpiderName, "Faida", kMindName});
    auto define_submodule = [&ind_algos_module, &main_module](char const* name,
                                                              std::vector<char const*> algorithms) {
        auto algos_module = main_module.def_submodule(name).def_submodule("algorithms");
        for (auto algo_name : algorithms) {
            algos_module.attr(algo_name) = ind_algos_module.attr(algo_name);
        }
        algos_module.attr("Default") = algos_module.attr(algorithms.front());
    };

    define_submodule("aind", {kSpiderName, kMindName});
}
}  // namespace python_bindings
