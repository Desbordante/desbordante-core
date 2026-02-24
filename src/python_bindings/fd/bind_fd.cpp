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
using PySelectionPairs = py::typing::Iterable<py::typing::Tuple<py::int_, py::int_>>;

#define FD_CLASS_NAME "FunctionalDependency"
#define ATTRIBUTE_CLASS_NAME "Attribute"
#define MULTI_ATTR_RHS_FD_STORAGE_CLASS_NAME "MultiAttrRhsFdStorage"

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

FdList FdsToList(algos::MultiAttrRhsFdStorage const& storage) {
    // The container would be allocated twice if we created it on
    // core's side: once there, and the other time for the copy.
    FdList fd_list{storage.GetStripped().size()};
    Py_ssize_t i = 0;
    for (model::FunctionalDependency fd : storage) {
        // If not released, the refcount of the new object will reach 0 after
        // this line, triggering UB on access.
        PyList_SET_ITEM(fd_list.ptr(), i, py::cast(std::move(fd)).release().ptr());
        ++i;
    }
    return fd_list;
}

std::deque<algos::MultiAttrRhsStrippedFd> StrippedFdsFromIntPairs(PySelectionPairs int_pairs,
                                                                  std::size_t col_number) {
    std::deque<algos::MultiAttrRhsStrippedFd> stripped_fds;
    for (py::handle pair : int_pairs) {
        if (!py::isinstance<py::tuple>(pair)) {
            throw std::runtime_error("FD in storage must be a tuple!");
        }
        auto tuple_pair = py::cast<py::tuple>(pair);
        if (tuple_pair.size() != 2) {
            throw std::runtime_error("FD in storage must be a pair!");
        }
        py::object lhs{tuple_pair[0]};
        if (!py::isinstance<py::int_>(lhs)) {
            throw std::runtime_error("LHS column selection must be an int!");
        }
        py::object rhs{tuple_pair[1]};
        if (!py::isinstance<py::int_>(rhs)) {
            throw std::runtime_error("RHS column selection must be an int!");
        }
        stripped_fds.emplace_back(python_bindings::IntToBitset(lhs, col_number),
                                  python_bindings::IntToBitset(rhs, col_number));
    }
    return stripped_fds;
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
            .def("to_input", &model::FunctionalDependency::ToInput)
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
    py::class_<MultiAttrRhsFdStorage, MultiAttrRhsFdStorage::OwningPointer>(
            fd_module, MULTI_ATTR_RHS_FD_STORAGE_CLASS_NAME)
            .def("to_fds", static_cast<FdList (*)(MultiAttrRhsFdStorage const&)>(FdsToList))
            .def(
                    "__iter__",
                    [](MultiAttrRhsFdStorage const& storage) {
                        return py::make_iterator(storage.begin(), storage.end());
                    },
                    py::keep_alive<0, 1>())
            .def(py::pickle(
                    // __getstate__
                    [](MultiAttrRhsFdStorage const& storage) {
                        auto const& stripped_fds = storage.GetStripped();
                        model::TableHeader const& header = storage.GetTableHeader();
                        py::list py_stripped_fds{stripped_fds.size()};
                        Py_ssize_t i = 0;
                        for (MultiAttrRhsStrippedFd const& stripped_fd : stripped_fds) {
                            // If not released, the refcount of the new object will reach 0 after
                            // this line, triggering UB on access.
                            PyList_SET_ITEM(py_stripped_fds.ptr(), i,
                                            py::make_tuple(BitsetToInt(stripped_fd.lhs),
                                                           BitsetToInt(stripped_fd.rhs))
                                                    .release()
                                                    .ptr());
                            ++i;
                        }
                        return py::make_tuple(
                                py::make_tuple(header.table_name, header.column_names),
                                std::move(py_stripped_fds));
                    },
                    // __setstate__
                    [](py::tuple t) {
                        if (t.size() != 2) {
                            throw std::runtime_error(MULTI_ATTR_RHS_FD_STORAGE_CLASS_NAME
                                                     " pickle state must have two elements!");
                        }
                        py::tuple header_tuple = t[0];
                        if (!py::isinstance<py::tuple>(header_tuple)) {
                            throw std::runtime_error(
                                    "Expected tuple as first element "
                                    "in " MULTI_ATTR_RHS_FD_STORAGE_CLASS_NAME " pickle state");
                        }
                        if (header_tuple.size() != 2) {
                            throw std::runtime_error("Header must be represented by two elements!");
                        }
                        auto table_name = header_tuple[0].cast<std::string>();
                        auto column_names = header_tuple[1].cast<std::vector<std::string>>();
                        std::size_t const col_number = column_names.size();
                        py::object fds_list = t[1];
                        return MultiAttrRhsFdStorage{
                                {std::move(table_name), std::move(column_names)},
                                StrippedFdsFromIntPairs(std::move(fds_list), col_number)};
                    }))
            .def(py::init([](std::string table_name, std::vector<std::string> column_names,
                             PySelectionPairs selection_pairs) {
                     std::size_t const col_number = column_names.size();
                     return MultiAttrRhsFdStorage{
                             {std::move(table_name), std::move(column_names)},
                             StrippedFdsFromIntPairs(selection_pairs, col_number)};
                 }),
                 "table_name"_a, "column_names"_a, "selection_pairs"_a);

    static constexpr auto kPyroName = "Pyro";
    static constexpr auto kTaneName = "Tane";
    static constexpr auto kPFDTaneName = "PFDTane";
    auto fd_algos_module =
            BindPrimitive<hyfd::HyFD, Aid, EulerFD, Depminer, DFD, FastFDs, FdMine, FUN, Pyro, Tane,
                          PFDTane>(fd_module, &FDAlgorithm::SortedFdList, "FdAlgorithm", "get_fds",
                                   {"HyFD", "Aid", "EulerFD", "Depminer", "DFD", "FastFDs",
                                    "FdMine", "FUN", kPyroName, kTaneName, kPFDTaneName});
    BindFdAlgorithm<FDep>(fd_algos_module, "FDep");

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
