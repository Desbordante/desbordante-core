#include "bind_fastadc.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/dc/FastADC/fastadc.h"
#include "py_util/bind_primitive.h"
#include "py_util/table_serialization.h"

namespace py = pybind11;
using namespace algos::fastadc;

namespace fastadc_serialization {

py::tuple SerializeColumnOperand(ColumnOperand const& operand) {
    return py::make_tuple(
        operand.GetColumn()->GetIndex(),
        operand.GetTuple()._to_string()
    );
}

ColumnOperand DeserializeColumnOperand(py::tuple t, std::shared_ptr<RelationalSchema const> schema) {
    auto col_index = t[0].cast<size_t>();
    auto tuple_str = t[1].cast<std::string>();
    auto tuple_type = ColumnOperandTuple::_from_string(tuple_str.c_str());
    return ColumnOperand(schema->GetColumn(col_index), tuple_type);
}

py::tuple SerializePredicate(PredicatePtr predicate) {
    return py::make_tuple(
        static_cast<int>(predicate->GetOperator().GetType()),
        SerializeColumnOperand(predicate->GetLeftOperand()),
        SerializeColumnOperand(predicate->GetRightOperand())
    );
}

PredicatePtr DeserializePredicate(py::tuple t, std::shared_ptr<RelationalSchema const> schema) {
    OperatorType op_type = static_cast<OperatorType>(t[0].cast<int>());
    ColumnOperand lhs = DeserializeColumnOperand(t[1].cast<py::tuple>(), schema);
    ColumnOperand rhs = DeserializeColumnOperand(t[2].cast<py::tuple>(), schema);
    return new Predicate(Operator(op_type), lhs, rhs);
}
} // namespace fastadc_serialization

namespace python_bindings {
void BindFastADC(py::module_& main_module) {
    using namespace algos;
    using DC = algos::fastadc::DenialConstraint;

    auto dc_module = main_module.def_submodule("dc");
    py::class_<DC>(dc_module, "DC")
        .def("__str__", &DC::ToString)
        .def("__repr__", &DC::ToString)
        .def(py::pickle(
                // __getstate__
                [](DenialConstraint const& dc) {
                    py::tuple schema_state = table_serialization::SerializeRelationalSchema(dc.GetSchema().get());
                    py::list plist;
                    for (PredicatePtr p : dc.GetPredicateSet()) {
                        plist.append(fastadc_serialization::SerializePredicate(p));
                    }
                    return py::make_tuple(schema_state, plist);
                },
                // __setstate__
                [](py::tuple t) {
                    if (t.size() != 2) {
                        throw std::runtime_error("Invalid state for DenialConstraint pickle!");
                    }
                    std::shared_ptr<RelationalSchema const> schema = table_serialization::DeserializeRelationalSchema(t[0].cast<py::tuple>());
                    auto provider = std::make_shared<PredicateIndexProvider>();
                    boost::dynamic_bitset<> bitset(kPredicateBits);
                    py::list pred_list = t[1].cast<py::list>();
                    for (auto h : pred_list) {
                        PredicatePtr p  = fastadc_serialization::DeserializePredicate(h.cast<py::tuple>(), schema);
                        size_t idx = provider->GetIndex(p);
                        bitset.set(idx);
                    }
                    return DenialConstraint(bitset, provider, schema);
                }));

    BindPrimitiveNoBase<dc::FastADC>(dc_module, "FastADC")
            .def("get_dcs", &dc::FastADC::GetDCs, py::return_value_policy::copy);
}
}  // namespace python_bindings
