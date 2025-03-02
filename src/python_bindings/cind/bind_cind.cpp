#include "bind_cind.h"

#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "algorithms/cind/cind_algorithm.h"
#include "algorithms/ind/mining_algorithms.h"
#include "cind/cind.hpp"
#include "cind/cind_algorithm.h"
#include "py_util/bind_primitive.h"

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
}  // namespace

namespace python_bindings {
void BindCind(py::module_& main_module) {
    using namespace model;
    using namespace algos::cind;

    auto cind_module = main_module.def_submodule("cind");
    py::class_<CIND>(cind_module, "CIND")
            .def("__str__", &CIND::ToString)
            .def("__eq__", [](CIND const& cind1, CIND const& cind2) { return cind1 == cind2; })
            .def("__hash__", [](CIND const& cind) { return py::hash(py::int_(cind.Hash())); })
            .def("to_pattern_tableau", [](CIND const& cind) {
                py::set result;
                for (auto const& cond : cind.conditions) {
                    result.add(VectorToTuple(cond.condition_attrs_values));
                }
                return result;
            }).def("get_condition_attributes", [](CIND const& cind) {
                return VectorToTuple(cind.conditional_attributes);
            });

    BindPrimitiveNoBase<CindAlgorithm>(cind_module, "Cind")
            .def("get_cinds", &CindAlgorithm::CINDList)
            .def("time_taken", &CindAlgorithm::TimeTaken);
}
}  // namespace python_bindings
