#include "bind_ac.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/algebraic_constraints/ac.h"
#include "algorithms/algebraic_constraints/mining_algorithms.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindAc(py::module_& main_module) {
    using namespace algos;

    auto ac_module = main_module.def_submodule("ac");
    py::class_<ACException>(ac_module, "ACException")
            .def_readonly("row_index", &ACException::row_i)
            .def_readonly("column_pairs", &ACException::column_pairs);
    py::class_<RangesCollection>(ac_module, "ACRanges")
            .def_property_readonly(
                    "column_indices",
                    [](RangesCollection const& ranges) { return ranges.col_pair.col_i; })
            .def_property_readonly("ranges", [](RangesCollection const& ranges) {
                std::vector<std::pair<pybind11::float_, pybind11::float_>> res;
                res.reserve(ranges.ranges.size() / 2);
                assert(ranges.ranges.size() % 2 == 0);
                for (size_t i = 0; i < ranges.ranges.size(); i += 2) {
                    // TODO: change this once a proper conversion mechanism from
                    // `model::INumericType` is implemented
                    std::string l_endpoint =
                            ranges.col_pair.num_type->ValueToString(ranges.ranges[i]);
                    std::string r_endpoint =
                            ranges.col_pair.num_type->ValueToString(ranges.ranges[i + 1]);
                    res.emplace_back(pybind11::float_(pybind11::str(l_endpoint)),
                                     pybind11::float_(pybind11::str(r_endpoint)));
                }
                return res;
            });
    BindPrimitiveNoBase<ACAlgorithm>(ac_module, "AcAlgorithm")
            .def("get_ac_ranges", &ACAlgorithm::GetRangesCollections,
                 py::return_value_policy::reference_internal)
            .def(
                    "get_ac_exceptions",
                    [](ACAlgorithm& algo) {
                        algo.CollectACExceptions();
                        return algo.GetACExceptions();
                    },
                    py::return_value_policy::reference_internal);
}
}  // namespace python_bindings
