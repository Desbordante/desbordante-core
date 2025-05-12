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

py::tuple SerializePredicateIndexProvider(std::shared_ptr<PredicateIndexProvider> provider) {
    py::list predicates_list;
    for (size_t i = 0; i < provider->Size(); ++i) {
        predicates_list.append(SerializePredicate(provider->GetObject(i)));
    }
    return py::make_tuple(predicates_list);
}

std::shared_ptr<PredicateIndexProvider> DeserializePredicateIndexProvider(py::tuple t, std::shared_ptr<RelationalSchema const> schema) {
    auto provider = std::make_shared<PredicateIndexProvider>();
    py::list predicates_list = t[0].cast<py::list>();
    for (auto handle : predicates_list) {
        py::tuple predicate_tuple = handle.cast<py::tuple>();
        PredicatePtr predicate = DeserializePredicate(predicate_tuple, schema);
        provider->GetIndex(predicate);
    }
    return provider;
}

py::tuple SerializeDynamicBitset(boost::dynamic_bitset<> const& bitset) {
    std::vector<bool> bits;
    bits.reserve(bitset.size());
    for (size_t i = 0; i < bitset.size(); ++i) {
        bits.push_back(bitset.test(i));
    }
    return py::make_tuple(bits, bitset.size());
}

boost::dynamic_bitset<> DeserializeDynamicBitset(py::tuple t) {
    auto bits = t[0].cast<std::vector<bool>>();
    auto size = t[1].cast<size_t>();
    boost::dynamic_bitset<> result(size);
    for (size_t i = 0; i < bits.size(); ++i) {
        result[i] = bits[i];
    }
    return result;
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
                auto schema_state = table_serialization::SerializeRelationalSchema(dc.GetSchema());
                auto provider_state = fastadc_serialization::SerializePredicateIndexProvider(dc.GetPredicateSet().provider);
                auto bitset_state = fastadc_serialization::SerializeDynamicBitset(dc.GetPredicateSet().GetBitset());
                return py::make_tuple(schema_state, provider_state, bitset_state);
            },
            // __setstate__
            [](py::tuple t) {
                if (t.size() != 3) {
                    throw std::runtime_error("Invalid state for DenialConstraint pickle!");
                }
                auto schema = table_serialization::DeserializeRelationalSchema(t[0].cast<py::tuple>());
                auto provider = fastadc_serialization::DeserializePredicateIndexProvider(t[1].cast<py::tuple>(), schema);
                auto bitset = fastadc_serialization::DeserializeDynamicBitset(t[2].cast<py::tuple>());
                return DenialConstraint(bitset, provider, schema);
            }));

    BindPrimitiveNoBase<dc::FastADC>(dc_module, "FastADC")
            .def("get_dcs", &dc::FastADC::GetDCs, py::return_value_policy::copy);
}
}  // namespace python_bindings
