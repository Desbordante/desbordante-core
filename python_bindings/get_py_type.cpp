#include "get_py_type.h"

#include <functional>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <Python.h>

#include "algorithms/ar_algorithm_enums.h"
#include "algorithms/metric/enums.h"

namespace py = pybind11;

namespace python_bindings {

template <typename CppType, PyTypeObject*... PyTypes>
static std::pair<std::type_index, std::function<py::tuple()>> const TypeTuplePair{
        std::type_index{typeid(CppType)},
        []() { return py::make_tuple(py::handle{reinterpret_cast<PyObject*>(PyTypes)}...); }};

py::tuple GetPyType(std::type_index type_index) {
    constexpr static PyTypeObject* const py_int = &PyLong_Type;
    constexpr static PyTypeObject* const py_bool = &PyBool_Type;
    constexpr static PyTypeObject* const py_float = &PyFloat_Type;
    constexpr static PyTypeObject* const py_str = &PyUnicode_Type;
    constexpr static PyTypeObject* const py_list = &PyList_Type;

    // Type indexes are mapped to Python type tuples. The first element in the
    // tuple is the type of the parameter itself, other elements are
    // context-dependent.
    // The tuples are created from Python API's raw pointers when needed as
    // storing pybind11's objects themselves statically is unpredictable and can
    // lead to errors related to garbage collection.
    static const std::unordered_map<std::type_index, std::function<py::tuple()>> type_map{
            TypeTuplePair<bool, py_bool>,
            TypeTuplePair<ushort, py_int>,
            TypeTuplePair<int, py_int>,
            TypeTuplePair<unsigned int, py_int>,
            TypeTuplePair<double, py_float>,
            TypeTuplePair<long double, py_float>,
            TypeTuplePair<algos::metric::Metric, py_str>,
            TypeTuplePair<algos::metric::MetricAlgo, py_str>,
            TypeTuplePair<algos::InputFormat, py_str>,
            // The second type denotes the type of list elements.
            TypeTuplePair<std::vector<unsigned int>, py_list, py_int>,
    };
    return type_map.at(type_index)();
}

}  // namespace python_bindings
