#include "python_bindings/sfd/bind_sfd.h"

#include <algorithm>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/fd/sfd/cords.h"
#include "core/algorithms/fd/sfd/correlation.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/table_serialization.h"

namespace {
namespace py = pybind11;

py::tuple SerializeCorrelation(algos::Correlation const& corr) {
    py::tuple lhs_state = table_serialization::SerializeColumn(corr.GetLhs());
    py::tuple rhs_state = table_serialization::SerializeColumn(corr.GetRhs());
    return py::make_tuple(std::move(lhs_state), std::move(rhs_state));
}

}  // namespace

namespace python_bindings {

void BindSFD(py::module_& main_module) {
    using namespace algos;
    auto sfd_module = main_module.def_submodule("sfd");

    // NOTE: technically `Correlation` will contain an invalid pointer to the schema if the
    // algorithm object is destroyed, but we do not use the schema anywhere.
    py::class_<Correlation>(sfd_module, "Correlation")
            .def("__str__", &Correlation::ToString)
            .def("to_string", &Correlation::ToString)
            .def("GetLhsIndex", &Correlation::GetLhsIndex)
            .def("GetRhsIndex", &Correlation::GetRhsIndex)
            .def("GetLhsName", &Correlation::GetLhsName)
            .def("GetRhsName", &Correlation::GetRhsName)
            .def("__eq__", [](Correlation const& corr1, Correlation const& corr2) {
                py::tuple corr1_state_tuple = SerializeCorrelation(corr1);
                py::tuple corr2_state_tuple = SerializeCorrelation(corr2);

                return corr1_state_tuple.equal(corr2_state_tuple);
            })
            .def("__hash__", [](Correlation const& corr) {
                py::tuple corr_state_tuple = SerializeCorrelation(corr);
                return py::hash(corr_state_tuple);
            })
            .def(py::pickle(
                    // __getstate__
                    [](Correlation const& corr) {
                        return SerializeCorrelation(corr);
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 2) {
                            throw std::runtime_error("Invalid state for Correlation pickle!");
                        }
                        std::shared_ptr<RelationalSchema> dummy_schema =
                                std::make_shared<RelationalSchema>("__dummy__");
                        Column lhs_col = table_serialization::DeserializeColumn(
                                t[0].cast<py::tuple>(), dummy_schema.get());
                        Column rhs_col = table_serialization::DeserializeColumn(
                                t[1].cast<py::tuple>(), dummy_schema.get());
                        return Correlation(std::move(lhs_col), std::move(rhs_col));
                    }));

    auto sfd_algorithms_module = sfd_module.def_submodule("algorithms");
    auto cls = py::class_<Cords, FDAlgorithm>(sfd_algorithms_module, "SFDAlgorithm")
                       .def(py::init<>())
                       .def("get_correlations", &Cords::GetCorrelations,
                            py::return_value_policy::copy);
    sfd_algorithms_module.attr("Default") = cls;
}
}  // namespace python_bindings
