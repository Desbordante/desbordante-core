#include <filesystem>

#include <easylogging++.h>
#include <pybind11/pybind11.h>

#include "algorithms/algorithms.h"
#include "model/ar.h"
#include "py_ar_algorithm.h"
#include "py_data_stats.h"
#include "py_fd_algorithm.h"
#include "py_fd_verifier.h"
#include "py_metric_verifier.h"

INITIALIZE_EASYLOGGINGPP

#define DEFINE_PRIMITIVE(type)                                                                    \
    py::class_<Py##type>(module, #type)                                                           \
            .def(py::init<>())                                                                    \
            .def("fit",                                                                           \
                 py::overload_cast<std::string const&, char, bool, py::kwargs const&>(            \
                         &Py##type::Fit),                                                         \
                 "path"_a, "separator"_a = ',', "has_header"_a = true, "Transform data from CSV") \
            .def("fit",                                                                           \
                 py::overload_cast<pybind11::object, std::string, py::kwargs const&>(             \
                         &Py##type::Fit),                                                         \
                 "df"_a, "name"_a = "Pandas dataframe", "Transform data from pandas dataframe")   \
            .def("execute", &Py##type::Execute, "Process data")
#define DEFINE_PRIMITIVE_WITH_RES(type) \
    DEFINE_PRIMITIVE(type).def("get_results", &Py##type::GetResults)

namespace python_bindings {

namespace py = pybind11;
using PyApriori = PyArAlgorithm<algos::Apriori>;
using PyTane = PyFDAlgorithm<algos::Tane>;
using PyPyro = PyFDAlgorithm<algos::Pyro>;
using PyFUN = PyFDAlgorithm<algos::FUN>;
using PyFdMine = PyFDAlgorithm<algos::Fd_mine>;
using PyFastFDs = PyFDAlgorithm<algos::FastFDs>;
using PyHyFD = PyFDAlgorithm<algos::hyfd::HyFD>;
using PyFDep = PyFDAlgorithm<algos::FDep>;
using PyDFD = PyFDAlgorithm<algos::DFD>;
using PyDepminer = PyFDAlgorithm<algos::Depminer>;
using PyAid = PyFDAlgorithm<algos::Aid>;
using FDHighlight = algos::fd_verifier::Highlight;
using model::ARStrings;

PYBIND11_MODULE(desbordante, module) {
    using namespace pybind11::literals;

    if (std::filesystem::exists("logging.conf")) {
        el::Loggers::configureFromGlobal("logging.conf");
    } else {
        el::Configurations conf;
        conf.set(el::Level::Global, el::ConfigurationType::Enabled, "false");
        el::Loggers::reconfigureAllLoggers(conf);
    }

    module.doc() = "A data profiling library";

    py::class_<ARStrings>(module, "AssociativeRule")
            .def("__str__", &ARStrings::ToString)
            .def_readonly("left", &ARStrings::left)
            .def_readonly("right", &ARStrings::right)
            .def_readonly("confidence", &ARStrings::confidence);

    py::class_<PyFD>(module, "FD")
            .def("__str__", &PyFD::ToString)
            .def("__repr__", &PyFD::ToString)
            .def_property_readonly("lhs_indices", &PyFD::GetLhs)
            .def_property_readonly("rhs_index", &PyFD::GetRhs);

    py::class_<FDHighlight>(module, "FDHighlight")
            .def_property_readonly("cluster", &FDHighlight::GetCluster)
            .def_property_readonly("num_distinct_rhs_values", &FDHighlight::GetNumDistinctRhsValues)
            .def_property_readonly("most_frequent_rhs_value_proportion",
                                   &FDHighlight::GetMostFrequentRhsValueProportion);

    DEFINE_PRIMITIVE(FDVerifier)
            .def("fd_holds", &PyFDVerifier::FDHolds)
            .def("get_error", &PyFDVerifier::GetError)
            .def("get_num_error_clusters", &PyFDVerifier::GetNumErrorClusters)
            .def("get_num_error_rows", &PyFDVerifier::GetNumErrorRows)
            .def("get_highlights", &PyFDVerifier::GetHighlights);

    DEFINE_PRIMITIVE_WITH_RES(DataStats);
    DEFINE_PRIMITIVE_WITH_RES(Apriori);
    DEFINE_PRIMITIVE_WITH_RES(Tane);
    DEFINE_PRIMITIVE_WITH_RES(Pyro);
    DEFINE_PRIMITIVE_WITH_RES(FUN);
    DEFINE_PRIMITIVE_WITH_RES(FdMine);
    DEFINE_PRIMITIVE_WITH_RES(FastFDs);
    DEFINE_PRIMITIVE_WITH_RES(HyFD);
    DEFINE_PRIMITIVE_WITH_RES(FDep);
    DEFINE_PRIMITIVE_WITH_RES(DFD);
    DEFINE_PRIMITIVE_WITH_RES(Depminer);
    DEFINE_PRIMITIVE_WITH_RES(Aid);
    DEFINE_PRIMITIVE_WITH_RES(MetricVerifier);
}
#undef DEFINE_PRIMITIVE_WITH_RES
#undef DEFINE_PRIMITIVE

}  // namespace python_bindings
