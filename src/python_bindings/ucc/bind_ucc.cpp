#include "python_bindings/ucc/bind_ucc.h"

#include <cstddef>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/ucc/mining_algorithms.h"
#include "core/algorithms/ucc/ucc.h"
#include "core/config/indices/type.h"
#include "core/model/table/column.h"
#include "core/util/bitset_utils.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/table_serialization.h"

namespace {
namespace py = pybind11;
using model::UCC;

py::tuple MakeUCCNameTuple(UCC const& ucc) {
    std::vector<Column const*> columns = ucc.GetColumns();
    py::tuple tuple{columns.size()};
    for (std::size_t i = 0; i < columns.size(); ++i) {
        tuple[i] = columns[i]->GetName();
    }
    return tuple;
}
}  // namespace

namespace python_bindings {
void BindUcc(py::module_& main_module) {
    using namespace algos;

    auto ucc_module = main_module.def_submodule("ucc");
    py::class_<UCC>(ucc_module, "UCC")
            .def("__str__", &UCC::ToIndicesString)
            .def("to_short_string", &UCC::ToIndicesString)
            .def("to_long_string", &UCC::ToString)
            .def_property_readonly("indices", &UCC::GetColumnIndicesAsVector)
            .def("__eq__", [](UCC const& a, UCC const& b) { return a == b; })
            .def("__hash__", [](UCC const& ucc) { return py::hash(MakeUCCNameTuple(ucc)); })
            .def(py::pickle(
                    // __getstate__
                    [](UCC const& ucc) {
                        py::tuple schema_state = table_serialization::SerializeRelationalSchema(
                                ucc.GetSchema().get());
                        py::tuple vertical_state = table_serialization::SerializeVertical(ucc);
                        return py::make_tuple(std::move(schema_state), std::move(vertical_state));
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 2) {
                            throw std::runtime_error("Invalid state for UCC pickle!");
                        }
                        py::tuple schema_state = t[0].cast<py::tuple>();
                        std::shared_ptr<RelationalSchema const> schema_ptr =
                                table_serialization::DeserializeRelationalSchema(schema_state);
                        py::tuple vertical_state = t[1].cast<py::tuple>();
                        Vertical vert = table_serialization::DeserializeVertical(vertical_state,
                                                                                 schema_ptr.get());
                        return UCC(std::move(schema_ptr), std::move(vert));
                    }));
    BindPrimitive<HPIValid, HyUCC, PyroUCC>(
            ucc_module, py::overload_cast<>(&UCCAlgorithm::UCCList, py::const_), "UccAlgorithm",
            "get_uccs", {"HPIValid", "HyUCC", "PyroUCC"}, pybind11::return_value_policy::copy);
}
}  // namespace python_bindings
