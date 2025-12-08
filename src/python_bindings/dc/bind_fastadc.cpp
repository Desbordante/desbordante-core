#include "python_bindings/dc/bind_fastadc.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/dc/FastADC/fastadc.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/table_serialization.h"

namespace py = pybind11;
using namespace algos::fastadc;

namespace fastadc_serialization {

py::tuple SerializeColumnOperand(ColumnOperand const& operand) {
    return py::make_tuple(operand.GetColumn()->GetIndex(), operand.GetTuple()._to_string());
}

ColumnOperand DeserializeColumnOperand(py::tuple t,
                                       std::shared_ptr<RelationalSchema const> schema) {
    if (t.size() != 2) throw std::runtime_error("Invalid state for ColumnOperand pickle!");
    auto col_index = t[0].cast<size_t>();
    auto tuple_str = t[1].cast<std::string>();
    auto tuple_type = ColumnOperandTuple::_from_string(tuple_str.c_str());
    return ColumnOperand(schema->GetColumn(col_index), tuple_type);
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

namespace {
    using DC = algos::fastadc::DenialConstraint;
    py::tuple GetSortedPredicatesAsTuple(DC const& dc) {
        std::vector<PredicatePtr> predicate_ptrs;
        for (PredicatePtr predicate : dc.GetPredicateSet()){
            predicate_ptrs.push_back(predicate);
        }
        std::sort(predicate_ptrs.begin(), predicate_ptrs.end(),
        [](PredicatePtr const& p1, PredicatePtr const& p2){
            return p1->ToString() < p2->ToString();
        });
        py::tuple predicates(predicate_ptrs.size());
        for (size_t i = 0; i < predicate_ptrs.size(); i++){
            predicates[i] = fastadc_serialization::SerializePredicate(predicate_ptrs[i]);
        } 
        return predicates;
    }
} // namespace

namespace python_bindings {
void BindFastADC(py::module_& main_module) {
    using namespace algos;
    using DC = algos::fastadc::DenialConstraint;

    auto dc_module = main_module.def_submodule("dc");
    py::class_<DC>(dc_module, "DC")
            .def("__str__", &DC::ToString)
            .def("__repr__", &DC::ToString)
            .def("__eq__", [](DC const& dc1, DC const& dc2){
                if (&dc1 == &dc2){
                    return true;
                }
                if (*dc1.GetSchema() != *dc2.GetSchema()){
                    return false;
                }

                py::tuple dc1_predicates = GetSortedPredicatesAsTuple(dc1);
                py::tuple dc2_predicates = GetSortedPredicatesAsTuple(dc2);

                return dc1_predicates.equal(dc2_predicates);
            })
            .def("__hash__", [](DC const& dc){
                py::tuple dc_predicates_tuple = GetSortedPredicatesAsTuple(dc);
                py::tuple schema_tuple = table_serialization::ConvertSchemaToImmutableTuple(dc.GetSchema().get());

                py::tuple state_tuple = py::make_tuple(
                    std::move(dc_predicates_tuple),
                    std::move(schema_tuple)
                );

                return py::hash(state_tuple);
            })
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
