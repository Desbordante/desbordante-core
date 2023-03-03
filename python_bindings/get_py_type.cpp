#include "get_py_type.h"

#include <functional>
#include <iostream>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <Python.h>

#include "algorithms/ar_algorithm_enums.h"
#include "algorithms/metric/enums.h"

namespace py = pybind11;

#define CPP_TYPE(type_name) std::type_index{typeid(type_name)}
#define TYPE_PAIR(cpp_type, ...) {CPP_TYPE(cpp_type), MakeTypeTupleMaker(__VA_ARGS__)}

namespace python_bindings {

// Create a function returning a tuple of Python types from pointers to
// PyTypeObject.
template <typename... Types>
std::function<py::tuple()> MakeTypeTupleMaker(Types... types) {
    return [types...]() -> py::tuple {
        return py::make_tuple(py::handle{reinterpret_cast<PyObject *>(types)}...);
    };
}

py::tuple GetPyType(std::type_index type_index) {
    static auto *const py_int = &PyLong_Type;
    static auto *const py_bool = &PyBool_Type;
    static auto *const py_float = &PyFloat_Type;
    static auto *const py_str = &PyUnicode_Type;
    static auto *const py_list = &PyList_Type;

    // Type indexes are mapped to Python type tuples. The first element in the
    // tuple is the type of the parameter itself, other elements are
    // context-dependent.
    // The tuples are only created when needed as storing Python objects
    // statically is unpredictable and can lead to errors.
    static const std::unordered_map<std::type_index, std::function<py::tuple()>> type_map{
            TYPE_PAIR(bool, py_bool),
            TYPE_PAIR(ushort, py_int),
            TYPE_PAIR(int, py_int),
            TYPE_PAIR(unsigned int, py_int),
            TYPE_PAIR(double, py_float),
            TYPE_PAIR(long double, py_float),
            TYPE_PAIR(algos::metric::Metric, py_str),
            TYPE_PAIR(algos::metric::MetricAlgo, py_str),
            TYPE_PAIR(algos::InputFormat, py_str),
            // The second type denotes the type of list elements.
            TYPE_PAIR(std::vector<unsigned int>, py_list, py_int),
    };
    return type_map.at(type_index)();
}
#undef TYPE_PAIR
#undef CPP_TYPE

}  // namespace python_bindings
