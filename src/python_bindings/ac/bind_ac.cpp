#include "python_bindings/ac/bind_ac.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/algebraic_constraints/ac.h"
#include "core/algorithms/algebraic_constraints/mining_algorithms.h"
#include "core/model/types/create_type.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
std::unique_ptr<model::INumericType> CreateNumericTypeFromTypeId(model::TypeId type_id) {
    std::unique_ptr<model::Type> type_ptr = model::CreateType(type_id, false);
    return std::unique_ptr<model::INumericType>(
            dynamic_cast<model::INumericType*>(type_ptr.release()));
}

namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindAc(py::module_& main_module) {
    using namespace algos;

    auto ac_module = main_module.def_submodule("ac");
    py::class_<ACException>(ac_module, "ACException")
            .def_readonly("row_index", &ACException::row_i)
            .def_readonly("column_pairs", &ACException::column_pairs)
            .def(py::pickle(
                    // __getstate__
                    [](ACException const& exc) {
                        return py::make_tuple(exc.row_i, exc.column_pairs);
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 2) {
                            throw std::runtime_error("Invalid state for ACException pickle!");
                        }
                        size_t row_index = t[0].cast<size_t>();
                        auto column_pairs = t[1].cast<std::vector<std::pair<size_t, size_t>>>();
                        return ACException(row_index, std::move(column_pairs));
                    }));

    py::class_<RangesCollection>(ac_module, "ACRanges")
            .def_property_readonly(
                    "column_indices",
                    [](RangesCollection const& ranges) { return ranges.col_pair.col_i; })
            .def_property_readonly(
                    "ranges",
                    [](RangesCollection const& ranges) {
                        std::vector<std::pair<pybind11::float_, pybind11::float_>> res;
                        res.reserve(ranges.ranges.size() / 2);
                        assert(ranges.ranges.size() % 2 == 0);
                        for (size_t i = 0; i < ranges.ranges.size(); i += 2) {
                            // TODO: change this once a proper conversion mechanism from
                            // `model::INumericType` is implemented
                            std::string l_endpoint =
                                    ranges.col_pair.num_type->ValueToString(ranges.ranges[i]);
                            std::string r_endpoint =
                                    ranges.col_pair.num_type->ValueToString(ranges.ranges[i + 1]);
                            res.emplace_back(pybind11::float_(pybind11::str(l_endpoint)),
                                             pybind11::float_(pybind11::str(r_endpoint)));
                        }
                        return res;
                    })
            .def(py::pickle(
                    // __getstate__
                    [](RangesCollection const& obj) {
                        auto col_indices = obj.col_pair.col_i;
                        int type_id_int = static_cast<int>(obj.col_pair.num_type->GetTypeId());
                        std::vector<std::string> s_ranges;
                        s_ranges.reserve(obj.ranges.size());
                        for (auto ptr : obj.ranges) {
                            s_ranges.push_back(obj.col_pair.num_type->ValueToString(ptr));
                        }
                        return py::make_tuple(std::move(col_indices), type_id_int,
                                              std::move(s_ranges));
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 3) {
                            throw std::runtime_error("Invalid state for ACRanges pickle!");
                        }
                        auto col_indices = t[0].cast<std::pair<size_t, size_t>>();
                        int type_id_int = t[1].cast<int>();
                        auto s_ranges = t[2].cast<std::vector<std::string>>();
                        model::TypeId type_id = model::TypeId::_from_integral(type_id_int);
                        std::unique_ptr<model::INumericType> num_type =
                                CreateNumericTypeFromTypeId(type_id);
                        std::vector<std::byte const*> ranges;
                        ranges.reserve(s_ranges.size());
                        for (auto const& s : s_ranges) {
                            size_t size = num_type->GetSize();
                            std::byte* buf = new std::byte[size];
                            num_type->ValueFromStr(buf, s);
                            ranges.push_back(buf);
                        }
                        return RangesCollection(std::move(num_type), std::move(ranges),
                                                col_indices.first, col_indices.second);
                    }));

    BindPrimitiveNoBase<ACAlgorithm>(ac_module, "AcAlgorithm")
            .def("get_ac_ranges", &ACAlgorithm::GetRangesCollections,
                 py::return_value_policy::reference_internal)
            .def(
                    "get_ac_exceptions",
                    [](ACAlgorithm& algo) {
                        algo.CollectACExceptions();
                        return algo.GetACExceptions();
                    },
                    py::return_value_policy::reference_internal);
}
}  // namespace python_bindings
