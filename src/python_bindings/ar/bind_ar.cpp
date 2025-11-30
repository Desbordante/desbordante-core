#include "python_bindings/ar/bind_ar.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/association_rules/ar.h"
#include "core/algorithms/association_rules/mining_algorithms.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {
void BindAr(py::module_& main_module) {
    using namespace algos;
    using model::ArIDs;
    using model::ARStrings;

    auto ar_module = main_module.def_submodule("ar");
    py::class_<ARStrings>(ar_module, "ARStrings")
            .def("__str__", &ARStrings::ToString)
            .def_readonly("left", &ARStrings::left)
            .def_readonly("right", &ARStrings::right)
            .def_readonly("confidence", &ARStrings::confidence)
            .def_readonly("support", &ARStrings::support)
            .def(py::pickle(
                    // __getstate__
                    [](ARStrings const& ars) {
                        std::vector<std::string> left_vec(ars.left.begin(), ars.left.end());
                        std::vector<std::string> right_vec(ars.right.begin(), ars.right.end());
                        return py::make_tuple(std::move(left_vec), std::move(right_vec),
                                              ars.confidence, ars.support);
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 4) {
                            throw std::runtime_error("Invalid state for ARStrings pickle!");
                        }
                        auto left_vec = t[0].cast<std::vector<std::string>>();
                        auto right_vec = t[1].cast<std::vector<std::string>>();
                        double conf = t[2].cast<double>();
                        double supp = t[3].cast<double>();
                        std::list<std::string> left_list(left_vec.begin(), left_vec.end());
                        std::list<std::string> right_list(right_vec.begin(), right_vec.end());
                        return ARStrings(std::move(left_list), std::move(right_list), conf, supp);
                    }));

    py::class_<ArIDs>(ar_module, "ArIDs")
            .def_readonly("left", &ArIDs::left)
            .def_readonly("right", &ArIDs::right)
            .def_readonly("confidence", &ArIDs::confidence)
            .def_readonly("support", &ArIDs::support)
            .def(py::pickle(
                    // __getstate__
                    [](ArIDs const& arids) {
                        return py::make_tuple(arids.left, arids.right, arids.confidence,
                                              arids.support);
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 4) {
                            throw std::runtime_error("Invalid state for ArIDs pickle!");
                        }
                        auto left = t[0].cast<std::vector<unsigned>>();
                        auto right = t[1].cast<std::vector<unsigned>>();
                        double conf = t[2].cast<double>();
                        double supp = t[3].cast<double>();
                        return ArIDs(std::move(left), std::move(right), conf, supp);
                    }));

    py::class_<ARAlgorithm, Algorithm>(ar_module, "ArAlgorithm")
            .def("get_ars", &ARAlgorithm::GetArStringsList, py::return_value_policy::move)
            .def("get_itemnames", &ARAlgorithm::GetItemNamesVector)
            .def("get_ar_ids", &ARAlgorithm::GetArIDsList);

    auto algos_module = ar_module.def_submodule("algorithms");
    auto default_algorithm =
            detail::RegisterAlgorithm<Apriori, ARAlgorithm>(algos_module, "Apriori");
    algos_module.attr("Default") = default_algorithm;

    // Perhaps in the future there will be a need for:
    // default_algorithm.def("get_frequent_list", &Apriori::GetFrequentList);
}
}  // namespace python_bindings
