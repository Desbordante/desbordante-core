#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/nar/mining_algorithms.h"
#include "algorithms/nar/nar.h"
#include "algorithms/nar/value_range.h"
#include "py_util/bind_primitive.h"

namespace py = pybind11;

namespace nar_serialization {

py::object SerializeValueRange(std::shared_ptr<model::ValueRange> const& vr) {
    int type_code = static_cast<int>(vr->GetTypeId());
    if (static_cast<int>(vr->GetTypeId()) == static_cast<int>(model::TypeId::kString)) {
        auto svr = std::dynamic_pointer_cast<model::StringValueRange>(vr);
        return py::make_tuple(type_code, svr->domain);
    } else if (static_cast<int>(vr->GetTypeId()) == static_cast<int>(model::TypeId::kDouble)) {
        auto nvr = std::dynamic_pointer_cast<model::NumericValueRange<double>>(vr);
        return py::make_tuple(type_code, py::make_tuple(nvr->lower_bound, nvr->upper_bound));
    } else if (static_cast<int>(vr->GetTypeId()) == static_cast<int>(model::TypeId::kInt)) {
        auto nvr = std::dynamic_pointer_cast<model::NumericValueRange<model::Int>>(vr);
        return py::make_tuple(type_code, py::make_tuple(nvr->lower_bound, nvr->upper_bound));
    }
    throw std::runtime_error("Unsupported ValueRange type in serialization.");
}

std::shared_ptr<model::ValueRange> DeserializeValueRange(py::object const& obj) {
    auto tup = obj.cast<py::tuple>();
    int type_code = tup[0].cast<int>();
    model::TypeId type_id = model::TypeId::_from_integral(type_code);
    if (static_cast<int>(type_id) == static_cast<int>(model::TypeId::kString)) {
        auto domain = tup[1].cast<std::vector<std::string>>();
        return std::make_shared<model::StringValueRange>(domain);
    } else if (static_cast<int>(type_id) == static_cast<int>(model::TypeId::kDouble)) {
        auto bounds = tup[1].cast<py::tuple>();
        double lb = bounds[0].cast<double>();
        double ub = bounds[1].cast<double>();
        return std::make_shared<model::NumericValueRange<double>>(lb, ub);
    } else if (static_cast<int>(type_id) == static_cast<int>(model::TypeId::kInt)) {
        auto bounds = tup[1].cast<py::tuple>();
        model::Int lb = bounds[0].cast<model::Int>();
        model::Int ub = bounds[1].cast<model::Int>();
        return std::make_shared<model::NumericValueRange<model::Int>>(lb, ub);
    }
    throw std::runtime_error("Unsupported ValueRange type in deserialization.");
}

py::dict SerializeRangeMap(std::unordered_map<size_t, std::shared_ptr<model::ValueRange>> const& map) {
    py::dict d;
    for (auto const& [feature_idx, vr_ptr] : map) {
        d[py::int_(feature_idx)] = SerializeValueRange(vr_ptr);
    }
    return d;
}

std::unordered_map<size_t, std::shared_ptr<model::ValueRange>> DeserializeRangeMap(py::dict const& d) {
    std::unordered_map<size_t, std::shared_ptr<model::ValueRange>> map;
    for (auto item : d) {
        size_t feature_idx = item.first.cast<size_t>();
        auto vr = DeserializeValueRange(py::reinterpret_borrow<py::object>(item.second));
        map[feature_idx] = vr;
    }
    return map;
}
} // namespace nar_serialization

namespace python_bindings {

void BindNar(py::module_& main_module) {
    using namespace algos;
    using namespace algos::des;
    using namespace model;

    auto nar_module = main_module.def_submodule("nar");

    py::class_<ValueRange, std::shared_ptr<ValueRange>>(nar_module, "ValueRange")
        .def("__str__", &ValueRange::ToString)
        .def_property_readonly("type", &ValueRange::GetTypeId);

    py::class_<StringValueRange, ValueRange, std::shared_ptr<StringValueRange>>(nar_module, "StringValueRange")
        .def("__str__", &StringValueRange::ToString)
        .def_readonly("string", &StringValueRange::domain);

    py::class_<NumericValueRange<double>, ValueRange, std::shared_ptr<NumericValueRange<double>>>
        (nar_module, "FloatValueRange")
        .def("__str__", &NumericValueRange<double>::ToString)
        .def_readonly("lower_bound", &NumericValueRange<double>::lower_bound)
        .def_readonly("upper_bound", &NumericValueRange<double>::upper_bound);

    py::class_<NumericValueRange<model::Int>, ValueRange, std::shared_ptr<NumericValueRange<model::Int>>>
        (nar_module, "IntValueRange")
        .def("__str__", &NumericValueRange<model::Int>::ToString)
        .def_readonly("lower_bound", &NumericValueRange<model::Int>::lower_bound)
        .def_readonly("upper_bound", &NumericValueRange<model::Int>::upper_bound);

    py::class_<NARQualities>(nar_module, "NarQualities")
        .def("__str__", &NARQualities::ToString)
        .def_readonly("support", &NARQualities::support)
        .def_readonly("confidence", &NARQualities::confidence)
        .def_readonly("fitness", &NARQualities::fitness);

    py::class_<NAR>(nar_module, "NAR")
        .def("__str__", &NAR::ToString)
        .def_property_readonly("qualities", &NAR::GetQualities)
        .def_property_readonly("support", [](NAR const& n) { return n.GetQualities().support; })
        .def_property_readonly("confidence", [](NAR const& n) { return n.GetQualities().confidence; })
        .def_property_readonly("fitness", [](NAR const& n) { return n.GetQualities().fitness; })
        .def_property_readonly("ante", &NAR::GetAnte)
        .def_property_readonly("cons", &NAR::GetCons)
        .def(py::pickle(
            // __getstate__
            [](NAR const& nar) {
                auto quals = nar.GetQualities();
                py::dict ante_dict = nar_serialization::SerializeRangeMap(nar.GetAnte());
                py::dict cons_dict = nar_serialization::SerializeRangeMap(nar.GetCons());
                return py::make_tuple(quals.fitness, quals.support, quals.confidence,
                                      nar.IsQualitiesConsistent(), ante_dict, cons_dict);
            },
            // __setstate__
            [](py::tuple t) {
                if (t.size() != 6) {
                    throw std::runtime_error("Invalid state for NAR pickle!");
                }
                double fitness = t[0].cast<double>();
                double support = t[1].cast<double>();
                double confidence = t[2].cast<double>();
                bool qualities_consistent = t[3].cast<bool>();
                py::dict ante_dict = t[4].cast<py::dict>();
                py::dict cons_dict = t[5].cast<py::dict>();
                NAR nar;
                nar.SetQualitiesDirect(fitness, support, confidence);
                nar.SetQualitiesConsistent(qualities_consistent);
                auto ante_map = nar_serialization::DeserializeRangeMap(ante_dict);
                auto cons_map = nar_serialization::DeserializeRangeMap(cons_dict);
                for (auto const& p : ante_map) {
                    nar.InsertInAnte(p.first, p.second);
                }
                for (auto const& p : cons_map) {
                    nar.InsertInCons(p.first, p.second);
                }
                return nar;
            }));

    BindPrimitive<DES>(nar_module, &NARAlgorithm::GetNARVector, "NarAlgorithm", "get_nars", {"DES"},
                       pybind11::return_value_policy::copy);
}
}  // namespace python_bindings
