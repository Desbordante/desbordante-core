#include "python_bindings/dc/bind_fastadc.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/dc/FastADC/fastadc.h"
#include "core/util/enum_to_str.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/table_serialization.h"

namespace py = pybind11;
using namespace algos::fastadc;

namespace fastadc_serialization {

py::tuple SerializeColumnOperand(ColumnOperand const& operand) {
    return py::make_tuple(operand.GetColumn()->GetIndex(), util::EnumToStr(operand.GetTuple()));
}

ColumnOperand DeserializeColumnOperand(py::tuple t,
                                       std::shared_ptr<RelationalSchema const> schema) {
    if (t.size() != 2) throw std::runtime_error("Invalid state for ColumnOperand pickle!");
    auto col_index = t[0].cast<size_t>();
    auto tuple_str = t[1].cast<std::string>();
    auto tuple_type = util::EnumFromStr<ColumnOperandTuple>(tuple_str);
    if (!tuple_type) throw std::runtime_error("Invalid tuple type: " + tuple_str);
    return ColumnOperand(schema->GetColumn(col_index), *tuple_type);
}

py::tuple SerializePredicate(PredicatePtr predicate) {
    return py::make_tuple(static_cast<int>(predicate->GetOperator().GetType()),
                          SerializeColumnOperand(predicate->GetLeftOperand()),
                          SerializeColumnOperand(predicate->GetRightOperand()));
}

PredicatePtr DeserializePredicate(py::tuple t, std::shared_ptr<RelationalSchema const> schema) {
    OperatorType op_type = static_cast<OperatorType>(t[0].cast<int>());
    ColumnOperand lhs = DeserializeColumnOperand(t[1].cast<py::tuple>(), schema);
    ColumnOperand rhs = DeserializeColumnOperand(t[2].cast<py::tuple>(), std::move(schema));
    return new Predicate(Operator(op_type), lhs, rhs);
}
}  // namespace fastadc_serialization

namespace python_bindings {
void BindFastADC(py::module_& main_module) {
    using namespace algos;
    using DC = algos::fastadc::DenialConstraint;

    auto dc_module = main_module.def_submodule("dc");
    py::class_<DC>(dc_module, "DC")
            .def("__str__", &DC::ToString)
            .def("__repr__", &DC::ToString)
            .def(py::pickle(
                    //__getstate__
                    [](DenialConstraint const& dc) {
                        py::tuple schema_state = table_serialization::SerializeRelationalSchema(
                                dc.GetSchema().get());

                        py::list provider_state;
                        auto const& objs = dc.GetPredicateSet().provider->GetObjects();
                        for (PredicatePtr const& p : objs) {
                            provider_state.append(fastadc_serialization::SerializePredicate(p));
                        }

                        py::list bit_indices;
                        auto const& bs = dc.GetPredicateSet().GetBitset();
                        util::ForEachIndex(bs, [&bit_indices](size_t i) { bit_indices.append(i); });

                        return py::make_tuple(std::move(schema_state), std::move(provider_state),
                                              std::move(bit_indices));
                    },

                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 3) throw std::runtime_error("Invalid state for DC pickle!");

                        auto schema = table_serialization::DeserializeRelationalSchema(
                                t[0].cast<py::tuple>());

                        auto provider = std::make_shared<PredicateIndexProvider>();
                        for (auto h : t[1].cast<py::list>()) {
                            PredicatePtr p = fastadc_serialization::DeserializePredicate(
                                    h.cast<py::tuple>(), schema);
                            provider->GetIndex(p);
                        }

                        boost::dynamic_bitset<> bitset(provider->Size());
                        for (auto py_idx : t[2].cast<py::list>()) {
                            size_t idx = py_idx.cast<size_t>();
                            bitset.set(idx);
                        }

                        return DenialConstraint(bitset, std::move(provider), std::move(schema));
                    }));

    BindPrimitiveNoBase<dc::FastADC>(dc_module, "FastADC")
            .def("get_dcs", &dc::FastADC::GetDCs, py::return_value_policy::copy);
}
}  // namespace python_bindings
