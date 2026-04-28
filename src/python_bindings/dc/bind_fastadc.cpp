#include "python_bindings/dc/bind_fastadc.h"

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include "core/algorithms/dc/FastADC/fastadc.h"
#include "core/util/enum_to_str.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/table_serialization.h"
#include "python_bindings/py_util/vector_to_tuple.h"

namespace py = pybind11;
using namespace algos::fastadc;

namespace fastadc_serialization {
using DC = algos::fastadc::DenialConstraint;

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

py::tuple SerializeDc(DC const& dc) {
    py::tuple schema_state = table_serialization::SerializeRelationalSchema(dc.GetSchema().get());

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
}

DC DeserializeDc(py::tuple t) {
    if (t.size() != 3) throw std::runtime_error("Invalid state for DC pickle!");

    auto schema = table_serialization::DeserializeRelationalSchema(t[0].cast<py::tuple>());

    auto provider = std::make_shared<PredicateIndexProvider>();
    for (auto h : t[1].cast<py::list>()) {
        PredicatePtr p = fastadc_serialization::DeserializePredicate(h.cast<py::tuple>(), schema);
        provider->GetIndex(p);
    }

    boost::dynamic_bitset<> bitset(provider->Size());
    for (auto py_idx : t[2].cast<py::list>()) {
        size_t idx = py_idx.cast<size_t>();
        bitset.set(idx);
    }

    return DC(bitset, std::move(provider), std::move(schema));
}

py::tuple SerializePredicateSetCanonical(PredicateSet const& set) {
    std::vector<PredicatePtr> predicate_ptrs;
    for (PredicatePtr predicate : set) {
        predicate_ptrs.push_back(predicate);
    }
    std::ranges::sort(predicate_ptrs, [](auto const& p1, auto const& p2) {
        return p1->ToString() < p2->ToString();
    });
    return python_bindings::VectorToTuple(predicate_ptrs, [](auto elem) {
        return fastadc_serialization::SerializePredicate(elem);
    });
}

py::tuple ConvertDcToImmutableTuple(DC const& dc) {
    py::tuple dc_predicates_tuple = SerializePredicateSetCanonical(dc.GetPredicateSet());
    py::tuple schema_tuple =
            table_serialization::ConvertSchemaToImmutableTuple(dc.GetSchema().get());

    return py::make_tuple(std::move(dc_predicates_tuple), std::move(schema_tuple));
}
}  // namespace fastadc_serialization

namespace python_bindings {
using DC = algos::fastadc::DenialConstraint;

void BindFastADC(py::module_& main_module) {
    using namespace algos;

    auto dc_module = main_module.def_submodule("dc");
    py::class_<DC>(dc_module, "DC")
            .def("__str__", &DC::ToString)
            .def("__repr__", &DC::ToString)
            .def("__eq__",
                 [](DC const& dc1, DC const& dc2) {
                     if (&dc1 == &dc2) {
                         return true;
                     }
                     py::tuple tuple1 = fastadc_serialization::ConvertDcToImmutableTuple(dc1);
                     py::tuple tuple2 = fastadc_serialization::ConvertDcToImmutableTuple(dc2);

                     return tuple1.equal(tuple2);
                 })
            .def("__hash__",
                 [](DC const& dc) {
                     py::tuple state_tuple = fastadc_serialization::ConvertDcToImmutableTuple(dc);

                     return py::hash(state_tuple);
                 })
            .def(py::pickle(
                    //__getstate__
                    [](DC const& dc) { return fastadc_serialization::SerializeDc(dc); },

                    // __setstate__
                    [](py::tuple t) { return fastadc_serialization::DeserializeDc(t); }));

    BindPrimitiveNoBase<dc::FastADC>(dc_module, "FastADC")
            .def("get_dcs", &dc::FastADC::GetDCs, py::return_value_policy::copy);
}
}  // namespace python_bindings
