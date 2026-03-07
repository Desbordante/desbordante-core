#include "python_bindings/fd/bind_fd.h"

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/typing.h>

#include "core/algorithms/fd/fd.h"
#include "core/algorithms/fd/fd_algorithm.h"
#include "core/algorithms/fd/mining_algorithms.h"
#include "core/algorithms/fd/multi_attr_rhs_fd_storage.h"
#include "core/algorithms/fd/multi_attr_rhs_stripped_fd.h"
#include "core/config/indices/type.h"
#include "core/util/bitset_utils.h"
#include "python_bindings/py_util/bind_primitive.h"
#include "python_bindings/py_util/bitset_int_conv.h"
#include "python_bindings/py_util/table_serialization.h"

namespace {
namespace py = pybind11;

using FdList = py::typing::List<model::FunctionalDependency>;

#define FD_CLASS_NAME "FunctionalDependency"
#define ATTRIBUTE_CLASS_NAME "Attribute"

template <typename ElementType>
py::tuple VectorToTuple(std::vector<ElementType> vec) {
    std::size_t const size = vec.size();
    py::tuple tuple(size);
    for (std::size_t i = 0; i < size; ++i) {
        tuple[i] = std::move(vec[i]);
    }
    return tuple;
}

py::tuple MakeFdNameTuple(FD const& fd) {
    auto [lhs, rhs] = fd.ToNameTuple();
    return py::make_tuple(VectorToTuple(std::move(lhs)), std::move(rhs));
}

ssize_t AttributeHash(model::Attribute const& attr) {
    return py::hash(py::make_tuple(attr.name, attr.id));
}

std::string AttributeRepr(model::Attribute const& attr) {
    std::stringstream ss;
    ss << ATTRIBUTE_CLASS_NAME << '(';
    ss << py::repr(py::cast(attr.name));
    ss << ", ";
    ss << py::repr(py::cast(attr.id));
    ss << ')';
    return ss.str();
}

std::string AttributeStr(model::Attribute const& attr) {
    std::stringstream ss;
    ss << py::str(py::cast(attr.name));
    return ss.str();
}

std::string FdToStringFull(model::FunctionalDependency const& fd) {
    std::stringstream ss;
    ss << "FD on table " << fd.table_name << ": ";
    ss << "[";
    for (auto const& [name, id] : fd.lhs) {
        ss << "(" << name << ", " << id << ") ";
    }
    ss.seekp(-1, std::stringstream::cur);
    ss << "]";
    ss << " -> ";
    ss << "[";
    for (auto const& [name, id] : fd.rhs) {
        ss << "(" << name << ", " << id << ") ";
    }
    ss.seekp(-1, std::stringstream::cur);
    ss << "]";

    return ss.str();
}

std::string FdToStringShort(model::FunctionalDependency const& fd) {
    std::stringstream ss;
    ss << "[";
    for (model::Attribute const& attr : fd.lhs) {
        ss << AttributeStr(attr) << ' ';
    }
    ss.seekp(-1, std::stringstream::cur);
    ss << "]";
    ss << " -> ";
    ss << "[";
    for (model::Attribute const& attr : fd.rhs) {
        ss << AttributeStr(attr) << ' ';
    }
    ss.seekp(-1, std::stringstream::cur);
    ss << "]";
    return ss.str();
}

std::string FdRepr(model::FunctionalDependency const& fd) {
    std::stringstream ss;
    ss << FD_CLASS_NAME << '(';
    ss << py::repr(py::cast(fd.table_name));
    ss << ", ";
    ss << py::repr(py::cast(fd.lhs));
    ss << ", ";
    ss << py::repr(py::cast(fd.rhs));
    ss << ')';
    return ss.str();
}

ssize_t FdHash(model::FunctionalDependency const& fd) {
    return py::hash(py::make_tuple(fd.table_name, fd.lhs, fd.rhs));
}

template <typename Algo>
void BindFdAlgorithm(py::module const& fd_algos_module, char const* name) {
    python_bindings::detail::RegisterAlgorithm<Algo, algos::Algorithm>(fd_algos_module, name)
            .def("get_fd_storage", &Algo::GetFdStorage)
            .def("get_fds", [](Algo& algo) { return FdsToList(*algo.GetFdStorage()); });
}
}  // namespace

namespace python_bindings {
void BindFd(py::module_& main_module) {
    using namespace algos;
    using namespace pybind11::literals;

    auto fd_module = main_module.def_submodule("fd");
    py::class_<FD>(fd_module, "FD")
            .def("__str__", &FD::ToLongString)
            .def("to_long_string", &FD::ToLongString)
            .def("to_short_string", &FD::ToShortString)
            .def("to_index_tuple",
                 [](FD const& fd) {
                     return py::make_tuple(VectorToTuple(fd.GetLhsIndices()),
                                           std::move(fd.GetRhsIndex()));
                 })
            .def("to_name_tuple", MakeFdNameTuple)
            // TODO: implement proper equality check for FD
            .def("__eq__", [](FD const& fd1,
                              FD const& fd2) { return fd1.ToNameTuple() == fd2.ToNameTuple(); })
            .def("__hash__", [](FD const& fd) { return py::hash(MakeFdNameTuple(fd)); })
            .def_property_readonly("lhs_indices", &FD::GetLhsIndices)
            .def_property_readonly("rhs_index", &FD::GetRhsIndex)
            .def(py::pickle(
                    // __getstate__
                    [](FD const& fd) {
                        py::tuple schema_state = table_serialization::SerializeRelationalSchema(
                                fd.GetSchema().get());
                        py::tuple lhs_state = table_serialization::SerializeVertical(fd.GetLhs());
                        py::tuple rhs_state = table_serialization::SerializeColumn(fd.GetRhs());
                        return py::make_tuple(std::move(schema_state), std::move(lhs_state),
                                              std::move(rhs_state));
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 3) {
                            throw std::runtime_error("Invalid state for FD pickle!");
                        }
                        std::shared_ptr<RelationalSchema const> schema =
                                table_serialization::DeserializeRelationalSchema(
                                        t[0].cast<py::tuple>());
                        Vertical lhs = table_serialization::DeserializeVertical(
                                t[1].cast<py::tuple>(), schema.get());
                        Column rhs = table_serialization::DeserializeColumn(t[2].cast<py::tuple>(),
                                                                            schema.get());
                        return FD(lhs, rhs, std::move(schema));
                    }));

    py::class_<model::Attribute>(fd_module, ATTRIBUTE_CLASS_NAME)
            .def_readwrite("name", &model::Attribute::name)
            .def_readwrite("id", &model::Attribute::id)
            .def("__repr__", AttributeRepr)
            .def("__str__", AttributeStr)
            .def("__hash__", AttributeHash)
            .def("__iter__",
                 [](model::Attribute const& attr) {
                     return py::iter(py::make_tuple(attr.name, attr.id));
                 })
            .def(py::self == py::self)
            .def(py::self != py::self)
            .def(py::pickle(
                    [](model::Attribute const& attr) {  // __getstate__
                        return py::make_tuple(attr.name, attr.id);
                    },
                    [](py::tuple t) {  // __setstate__
                        if (t.size() != 2) {
                            throw std::runtime_error("Invalid state for " ATTRIBUTE_CLASS_NAME
                                                     " pickle!");
                        }
                        return model::Attribute(t[0].cast<std::string>(),
                                                t[1].cast<model::Index>());
                    }))
            .def(py::init<std::string, model::Index>(), "name"_a, "id"_a)
            .def(py::init([](std::tuple<std::string, model::Index> t) {
                     auto& [name, id] = t;
                     return model::Attribute(std::move(name), id);
                 }),
                 "name_id_tuple"_a);
    py::class_<model::FunctionalDependency>(fd_module, FD_CLASS_NAME)
            .def_readwrite("table_name", &model::FunctionalDependency::table_name)
            .def_readwrite("lhs", &model::FunctionalDependency::lhs)
            .def_readwrite("rhs", &model::FunctionalDependency::rhs)
            .def("__repr__", FdRepr)
            .def("__str__", FdToStringShort)
            .def("__hash__", FdHash)
            .def("__iter__",
                 [](model::FunctionalDependency const& fd) {
                     return py::iter(py::make_tuple(fd.table_name, fd.lhs, fd.rhs));
                 })
            .def(py::self == py::self)
            .def(py::self != py::self)
            .def("to_string_short", FdToStringShort)
            .def("to_string_full", FdToStringFull)
            .def(py::pickle(
                    // __getstate__
                    [](model::FunctionalDependency const& fd) {
                        return py::make_tuple(py::cast(fd.table_name), py::cast(fd.lhs),
                                              py::cast(fd.rhs));
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 3) {
                            throw std::runtime_error("Invalid state for " FD_CLASS_NAME " pickle!");
                        }
                        return model::FunctionalDependency(
                                t[0].cast<std::string>(),
                                t[1].cast<std::vector<model::Attribute>>(),
                                t[2].cast<std::vector<model::Attribute>>());
                    }))
            .def(py::init<std::string, std::vector<model::Attribute>,
                          std::vector<model::Attribute>>(),
                 "table_name"_a, "lhs"_a, "rhs"_a);

    static constexpr auto kPyroName = "Pyro";
    static constexpr auto kTaneName = "Tane";
    static constexpr auto kPFDTaneName = "PFDTane";
    auto fd_algos_module = BindPrimitive<hyfd::HyFD, Aid, EulerFD, Depminer, DFD, FastFDs, FDep,
                                         FdMine, FUN, Pyro, Tane, PFDTane>(
            fd_module, &FDAlgorithm::SortedFdList, "FdAlgorithm", "get_fds",
            {"HyFD", "Aid", "EulerFD", "Depminer", "DFD", "FastFDs", "FDep", "FdMine", "FUN",
             kPyroName, kTaneName, kPFDTaneName});

    auto define_submodule = [&fd_algos_module, &main_module](char const* name,
                                                             std::vector<char const*> algorithms) {
        auto algos_module = main_module.def_submodule(name).def_submodule("algorithms");
        for (auto algo_name : algorithms) {
            algos_module.attr(algo_name) = fd_algos_module.attr(algo_name);
        }
        algos_module.attr("Default") = algos_module.attr(algorithms.front());
    };

    define_submodule("afd", {kPyroName, kTaneName});
    define_submodule("pfd", {kPFDTaneName});
}
}  // namespace python_bindings
