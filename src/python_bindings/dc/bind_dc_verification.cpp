#include "bind_dc_verification.h"
#include "algorithms/dc/dc_verification.h"

#include "py_util/bind_primitive.h"

namespace python_bindings {
    namespace py = pybind11;
    using namespace algos;

    void BindDCVerification(py::module_ & main_module) {
        auto dc_verification_module = main_module.def_submodule("dc_verification");

        BindPrimitiveNoBase<DCVerification>(dc_verification_module, "DCVerification")
            .def("dc_holds", &DCVerification::DCHolds);
    }
}