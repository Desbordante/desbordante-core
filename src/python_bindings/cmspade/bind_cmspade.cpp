#include "python_bindings/cmspade/bind_cmspade.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/cmspade/cmspade.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace python_bindings{
void BindCMSpade(pybind11::module_& main_module){
    using namespace algos;
    using namespace algos::cmspade;

    auto cmspade_module = main_module.def_submodule("cmspade");

    pybind11::class_<Item>(cmspade_module, "Item")
        .def_property_readonly("id", &Item::GetId)
        .def("str", [](const Item& i) {
            return "Item(" + std::to_string(i.GetId()) + ")";
    });

    pybind11::class_<ItemAbstractionPair>(cmspade_module, "ItemAbstractionPair")
        .def_property_readonly("item", &ItemAbstractionPair::GetItem)
        .def_property_readonly("have_equal_relation", &ItemAbstractionPair::HaveEqualRelation)
        .def("__str__", [](const ItemAbstractionPair& p) {
            return "item = " + std::to_string(p.GetItem().GetId()) + 
                   ", equal = " + (p.HaveEqualRelation() ? "True" : "False");
    });
    pybind11::class_<Pattern>(cmspade_module, "Pattern")
        .def("get_support", &Pattern::GetSupport)
        .def("get_elements", &Pattern::GetElements)
        .def("__getitem__", [](const Pattern &p, size_t i) {
            if (i >= p.Size()) throw pybind11::index_error();
            return p.GetElements()[i];
        })
        .def("__len__", &Pattern::Size)
        .def("__str__", &Pattern::ToString);

    BindPrimitiveNoBase<CMSpade>(cmspade_module, "CMSpade")
        .def("get_frequent_patterns", &CMSpade::GetPatterns);
}
}