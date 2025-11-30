#include "python_bindings/nd/bind_nd.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/nd/nd.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace py = pybind11;

namespace {
template <typename ElementType>
py::tuple VectorToTuple(std::vector<ElementType> vec) {
    std::size_t const size = vec.size();
    py::tuple tuple(size);
    for (std::size_t i = 0; i < size; ++i) {
        tuple[i] = std::move(vec[i]);
    }
    return tuple;
}

py::tuple MakeNdNameTuple(model::ND const& nd) {
    auto [lhs, rhs, weight] = nd.ToNameTuple();
    return py::make_tuple(VectorToTuple(std::move(lhs)), VectorToTuple(std::move(rhs)), weight);
}
}  // namespace

namespace python_bindings {
void BindNd(py::module_& main_module) {
    using namespace model;
    auto nd_module = main_module.def_submodule("nd");
    py::class_<ND>(nd_module, "ND")
            .def("__str__", &ND::ToLongString)
            .def("to_short_string", &ND::ToShortString)
            .def("to_long_string", &ND::ToLongString)
            .def_property_readonly("lhs", &ND::GetLhs)
            .def_property_readonly("rhs", &ND::GetRhs)
            .def_property_readonly("lhs_indices", &ND::GetLhsIndices)
            .def_property_readonly("rhs_indices", &ND::GetRhsIndices)
            .def_property_readonly("weight", &ND::GetWeight)
            .def("__eq__", [](ND const& nd1, ND const& nd2) { return nd1 == nd2; })
            .def("__hash__", [](ND const& nd) { return py::hash(MakeNdNameTuple(nd)); });
}
}  // namespace python_bindings
