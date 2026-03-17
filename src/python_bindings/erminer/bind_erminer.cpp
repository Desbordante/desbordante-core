#include "bind_erminer.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/erminer/ERMiner.hpp"
#include "py_util/bind_primitive.h"

namespace python_bindings {

void BindERMiner(pybind11::module_& main_module) {
    using namespace algos;
    
    auto erminer_module = main_module.def_submodule("erminer");
    
    pybind11::class_<Rule>(erminer_module, "Rule")
            .def_readonly("antecedent", &Rule::antecedent)
            .def_readonly("consequent", &Rule::consequent)
            .def_readonly("support", &Rule::support)
            .def_readonly("confidence", &Rule::confidence)
            .def("__str__", &Rule::toString)
            .def("__repr__", &Rule::toString);
    
    BindPrimitiveNoBase<AlgoERMiner>(erminer_module, "ERMiner")
            .def(pybind11::init<>())
            .def("run_algorithm", 
                 pybind11::overload_cast<double, double, const std::string&, const std::string&>(
                     &AlgoERMiner::runAlgorithm),
                 pybind11::arg("min_support"),
                 pybind11::arg("min_confidence"),
                 pybind11::arg("input_file"),
                 pybind11::arg("output_file"))
            .def("run_algorithm",
                 pybind11::overload_cast<const std::string&, const std::string&, int, double>(
                     &AlgoERMiner::runAlgorithm),
                 pybind11::arg("input_file"),
                 pybind11::arg("output_file"),
                 pybind11::arg("relative_minsup"),
                 pybind11::arg("min_confidence"))
            .def("set_max_antecedent_size", &AlgoERMiner::setMaxAntecedentSize)
            .def("set_max_consequent_size", &AlgoERMiner::setMaxConsequentSize)
            .def("get_rules", &AlgoERMiner::getRules)
            .def("print_stats", &AlgoERMiner::printStats);
}

}  // namespace python_bindings