#include "bind_dynamic_algorithms.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/algo_factory.h"
#include "algorithms/dynamic/dynamic_algorithm.h"
#include "algorithms/dynamic/demo/demo_algo.h"
#include "config/names.h"
#include "config/tabular_data/crud_operations/operations.h"
#include "py_util/bind_primitive.h"
#include "py_util/py_to_any.h"

namespace {
namespace py = pybind11;
using algos::DynamicAlgorithm;
using algos::DynamicAlgorithmDemo;
using python_bindings::PyToAny;
auto const kVoidIndex = std::type_index{typeid(void)};

void ConfigureAlgo(DynamicAlgorithm& algorithm, py::kwargs const& kwargs) {
    algos::ConfigureFromFunction(
            algorithm, [&kwargs, &algorithm](std::string_view option_name) -> boost::any {
                std::type_index type_index = algorithm.GetTypeIndex(option_name);
                assert(type_index != kVoidIndex);
                return kwargs.contains(option_name)
                               ? PyToAny(option_name, type_index, kwargs[option_name.data()])
                               : boost::any{};
            });
}

void SetOptionByName(DynamicAlgorithm& algorithm, 
                     std::string_view option_name, 
                     py::kwargs const& kwargs) {
    std::type_index type_index = algorithm.GetTypeIndex(option_name);
    assert(type_index != kVoidIndex);
    boost::any option_value = kwargs.contains(option_name)
                              ? PyToAny(option_name, type_index, kwargs[option_name.data()])
                              : boost::any{};
    algorithm.SetOption(option_name, option_value);
}
}  // namespace

namespace python_bindings {
void BindDynamicAlgorithms(py::module_& main_module) {
    using namespace algos;

    auto dyn_module = main_module.def_submodule("dynamic");
    BindPrimitiveNoBase<DynamicAlgorithmDemo>(dyn_module, "Demo")
        .def(py::init([](py::kwargs const& kwargs) {
                auto algo = std::make_unique<DynamicAlgorithmDemo>();
                ConfigureAlgo(*algo, kwargs);
                algo->Initialize();
                return algo;
        }))
        .def("process",
            [](DynamicAlgorithmDemo& algo, py::kwargs const& kwargs) {
                for (const std::string_view& option_name : CRUD_OPTIONS) {
                    SetOptionByName(algo, option_name, kwargs);
                }
                algo.ProcessBatch();
            },
            "Process algorithm with given batch of changes")
        .def("get_result", &DynamicAlgorithmDemo::GetResult, 
             pybind11::return_value_policy::reference_internal)
        .def("get_result_diff", &DynamicAlgorithmDemo::GetResultDiff, 
             pybind11::return_value_policy::reference_internal)
        ;

}
}  // namespace python_bindings
