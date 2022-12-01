#include <easylogging++.h>
#include <pybind11/pybind11.h>

#include "algorithms/algorithms.h"
#include "model/ar.h"
#include "py_ar_algorithm.h"
#include "py_csv_stats.h"
#include "py_fd_primitive.h"
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
using PyTane = PyFDPrimitive<algos::Tane>;
using PyPyro = PyFDPrimitive<algos::Pyro>;
using PyFUN = PyFDPrimitive<algos::FUN>;
using PyFdMine = PyFDPrimitive<algos::Fd_mine>;
using PyFastFDs = PyFDPrimitive<algos::FastFDs>;
using PyHyFD = PyFDPrimitive<algos::hyfd::HyFD>;
using PyFDep = PyFDPrimitive<algos::FDep>;
using PyDFD = PyFDPrimitive<algos::DFD>;
using PyDepminer = PyFDPrimitive<algos::Depminer>;
using PyAid = PyFDPrimitive<algos::Aid>;
using model::ARStrings;

PYBIND11_MODULE(desbordante, module) {
    using namespace pybind11::literals;

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

    DEFINE_PRIMITIVE_WITH_RES(CsvStats);
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
