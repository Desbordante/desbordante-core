#include "get_py_type.h"

#include <functional>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <Python.h>

#include "algorithms/ar_algorithm_enums.h"
#include "algorithms/metric/enums.h"
#include "util/config/tabular_data/input_table_type.h"

namespace py = pybind11;

namespace {

constexpr PyTypeObject* const py_int = &PyLong_Type;
constexpr PyTypeObject* const py_bool = &PyBool_Type;
constexpr PyTypeObject* const py_float = &PyFloat_Type;
constexpr PyTypeObject* const py_str = &PyUnicode_Type;
constexpr PyTypeObject* const py_list = &PyList_Type;
constexpr PyTypeObject* const py_tuple = &PyTuple_Type;

py::handle MakeType(PyTypeObject* py_type_ptr) {
    // Does the same as `&py_type_ptr->ob_base.ob_base`: Python API simulates
    // inheritance here. There is also the `_PyObject_CAST` macro that does a
    // C-style cast, but, as indicated by the underscore, it's an internal
    // Python macro, so it should be avoided.
    return reinterpret_cast<PyObject*>(py_type_ptr);
}

py::handle MakeType(PyObject* py_type_ptr) {
    return py_type_ptr;
}

template <typename... TypePtrs>
py::tuple MakeTypeTuple(TypePtrs... type_ptrs) {
    return py::make_tuple(MakeType(type_ptrs)...);
}

template <typename CppType, PyTypeObject*... PyTypes>
std::pair<std::type_index, std::function<py::frozenset()>> const SimplePyTypeInfoPair{
        std::type_index{typeid(CppType)},
        []() { return py::frozenset{py::make_tuple(MakeTypeTuple(PyTypes...))}; }};

py::frozenset GetPyInputTableType() {
    std::vector<py::tuple> types;
    try {
        PyObject* df_type = py::module::import("pandas").attr("DataFrame").ptr();
        types = {MakeTypeTuple(df_type), MakeTypeTuple(py_tuple, df_type, py_str),
                 MakeTypeTuple(py_tuple, py_str, py_str, py_bool)};
    } catch (py::error_already_set& e) {
        if (e.matches(PyExc_ImportError)) {
            types = {MakeTypeTuple(py_tuple, py_str, py_str, py_bool)};
        } else {
            throw;
        }
    }
    return {py::make_iterator(types.begin(), types.end())};
}

}  // namespace

namespace python_bindings {

py::frozenset GetPyType(std::type_index type_index) {
    // Type indexes are mapped to frozensets of Python type tuples. The first
    // element of all the tuples is the type of the parameter itself. If that
    // element is `list`, the following element denotes the type of its
    // elements. If that parameter is `tuple` then the following parameters
    // denote the types of their respective elements.
    // The tuples are created at runtime from Python API's raw pointers (when
    // possible) as storing pybind11's objects themselves statically is
    // unpredictable and can lead to errors related to garbage collection.
    static const std::unordered_map<std::type_index, std::function<py::frozenset()>> type_map{
            SimplePyTypeInfoPair<bool, py_bool>,
            SimplePyTypeInfoPair<ushort, py_int>,
            SimplePyTypeInfoPair<int, py_int>,
            SimplePyTypeInfoPair<unsigned int, py_int>,
            SimplePyTypeInfoPair<double, py_float>,
            SimplePyTypeInfoPair<long double, py_float>,
            SimplePyTypeInfoPair<algos::metric::Metric, py_str>,
            SimplePyTypeInfoPair<algos::metric::MetricAlgo, py_str>,
            SimplePyTypeInfoPair<algos::InputFormat, py_str>,
            SimplePyTypeInfoPair<std::vector<unsigned int>, py_list, py_int>,
            {typeid(util::config::InputTable), GetPyInputTableType},
    };
    return type_map.at(type_index)();
}

}  // namespace python_bindings
