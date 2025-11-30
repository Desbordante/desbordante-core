#include "python_bindings/dd/bind_split.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/mining_algorithms.h"
#include "core/util/create_dd.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindSplit(py::module_& main_module) {
    using namespace algos;
    using model::DDString;
    using model::DFStringConstraint;

    auto dd_module = main_module.def_submodule("dd");
    py::class_<model::DDString>(dd_module, "DD")
            .def(py::init(&util::dd::CreateDd))
            .def("__str__", &model::DDString::ToString)
            .def("__repr__", &model::DDString::ToString)
            .def(py::pickle(
                    // __getstate__
                    [](model::DDString const& dd_str) {
                        std::vector<py::tuple> left_vec;
                        for (model::DFStringConstraint const& c : dd_str.left) {
                            left_vec.push_back(py::make_tuple(c.column_name,
                                                              c.constraint.lower_bound,
                                                              c.constraint.upper_bound));
                        }
                        std::vector<py::tuple> right_vec;
                        for (model::DFStringConstraint const& c : dd_str.right) {
                            right_vec.push_back(py::make_tuple(c.column_name,
                                                               c.constraint.lower_bound,
                                                               c.constraint.upper_bound));
                        }
                        return py::make_tuple(std::move(left_vec), std::move(right_vec));
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 2) {
                            throw std::runtime_error("Invalid state for DDString pickle!");
                        }
                        auto left_vec = t[0].cast<std::vector<py::tuple>>();
                        auto right_vec = t[1].cast<std::vector<py::tuple>>();
                        std::list<model::DFStringConstraint> left_list;
                        for (auto const& item : left_vec) {
                            std::string col_name = item[0].cast<std::string>();
                            double lower_bound = item[1].cast<double>();
                            double upper_bound = item[2].cast<double>();
                            left_list.push_back(model::DFStringConstraint(
                                    std::move(col_name), lower_bound, upper_bound));
                        }
                        std::list<model::DFStringConstraint> right_list;
                        for (auto const& item : right_vec) {
                            std::string col_name = item[0].cast<std::string>();
                            double lower_bound = item[1].cast<double>();
                            double upper_bound = item[2].cast<double>();
                            right_list.push_back(model::DFStringConstraint(
                                    std::move(col_name), lower_bound, upper_bound));
                        }
                        return model::DDString{std::move(left_list), std::move(right_list)};
                    }));

    BindPrimitiveNoBase<dd::Split>(dd_module, "Split").def("get_dds", &dd::Split::GetDDStringList);
}
}  // namespace python_bindings
