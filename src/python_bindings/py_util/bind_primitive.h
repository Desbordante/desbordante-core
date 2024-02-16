#pragma once

#include <array>
#include <cstddef>
#include <type_traits>

#include <pybind11/pybind11.h>

#include "algorithms/algorithm.h"

namespace python_bindings {

namespace detail {
template <typename AlgoType>
std::string MakeDocString() {
    auto algo = AlgoType();
    std::stringstream docstring;
    docstring << "Options:\n";
    for (std::string_view option_name : algo.GetPossibleOptions()) {
        docstring << option_name << ": " << algo.GetDescription(option_name) << "\n";
    }
    return docstring.str();
}

template <typename AlgorithmType, typename BaseType>
auto RegisterAlgorithm(pybind11::module_ module, auto&& name) {
    namespace py = pybind11;
    // py::multiple_inheritance only needed for Pyro. May incur an unnecessary performance cost for
    // other classes, but whatever pybind11 manages is probably not a performance-critical part, so
    // it should be fine.
    auto cls_ = py::class_<AlgorithmType, BaseType>(module, std::move(name),
                                                    py::multiple_inheritance());
    cls_.doc() = MakeDocString<AlgorithmType>();
    cls_.def(py::init<>());
    return cls_;
}

template <typename T>
struct MemberPointerClassHelper;

template <typename Class, typename Type>
struct MemberPointerClassHelper<Type Class::*> {
    using type = Class;
};

template <typename MemberPointer>
using MemberPointerClass = typename MemberPointerClassHelper<MemberPointer>::type;
}  // namespace detail

template <typename Default, typename... Others>
auto BindPrimitive(pybind11::module_& module, auto result_method, char const* base_name,
                   char const* base_result_method_name,
                   std::array<char const*, sizeof...(Others) + 1> algo_names,
                   // UB if incorrect. If your algorithm base method is a simple
                   // util::PrimitiveCollection<...>::AsList() call, the default should work,
                   // otherwise consult pybind11's docs.
                   pybind11::return_value_policy result_rv_policy =
                           pybind11::return_value_policy::reference_internal) {
    namespace py = pybind11;
    using algos::Algorithm;

    using ResultMethodType = std::decay_t<decltype(result_method)>;
    static_assert(std::is_member_pointer_v<ResultMethodType>);
    using Base = detail::MemberPointerClass<ResultMethodType>;
    py::class_<Base, Algorithm>(module, base_name)
            .def(base_result_method_name, result_method, result_rv_policy);
    auto algos_module = module.def_submodule("algorithms");
    auto arr_iter = algo_names.begin();
    auto default_ = detail::RegisterAlgorithm<Default, Base>(algos_module, *arr_iter++);
    (detail::RegisterAlgorithm<Others, Base>(algos_module, *arr_iter++), ...);
    algos_module.attr("Default") = default_;
    return algos_module;
}

template <typename AlgorithmType>
auto BindPrimitiveNoBase(pybind11::module_& module, char const* algo_name) {
    namespace py = pybind11;
    using algos::Algorithm;

    auto algos_module = module.def_submodule("algorithms");
    auto default_ = detail::RegisterAlgorithm<AlgorithmType, Algorithm>(algos_module, algo_name);
    algos_module.attr("Default") = default_;
    return default_;
}

}  // namespace python_bindings
