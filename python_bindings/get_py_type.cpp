#include "get_py_type.h"

#include <functional>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <Python.h>

#include "algorithms/ar_algorithm_enums.h"
#include "algorithms/metric/enums.h"
#include "algorithms/relational_algorithm.h"

namespace py = pybind11;

namespace python_bindings {

constexpr static PyTypeObject* const py_int = &PyLong_Type;
constexpr static PyTypeObject* const py_bool = &PyBool_Type;
constexpr static PyTypeObject* const py_float = &PyFloat_Type;
constexpr static PyTypeObject* const py_str = &PyUnicode_Type;
constexpr static PyTypeObject* const py_list = &PyList_Type;
constexpr static PyTypeObject* const py_tuple = &PyTuple_Type;

template <typename T>
static py::handle MakeType(T* py_type_ptr) {
    static_assert(std::is_same_v<T*, PyTypeObject*> || std::is_same_v<T*, PyObject*>);
    return reinterpret_cast<PyObject*>(py_type_ptr);
}

template <typename... TypePtrs>
static py::tuple MakeTypeTuple(TypePtrs... type_ptrs) {
    return py::make_tuple(MakeType(type_ptrs)...);
}

template <typename CppType, PyTypeObject*... PyTypes>
static std::pair<std::type_index, std::function<py::frozenset()>> const SimplePyTypeInfoPair{
        std::type_index{typeid(CppType)},
        []() { return py::frozenset{py::make_tuple(MakeTypeTuple(PyTypes...))}; }};

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
            {typeid(algos::RelationStream), []() {
                 std::vector<py::tuple> types;
                 try {
                     PyObject* df_type = py::module::import("pandas").attr("DataFrame").ptr();
                     types = {MakeTypeTuple(df_type), MakeTypeTuple(py_tuple, df_type, py_str),
                              MakeTypeTuple(py_tuple, py_str),
                              MakeTypeTuple(py_tuple, py_str, py_str, py_bool)};
                 } catch (py::error_already_set&) {
                     types = {MakeTypeTuple(py_tuple, py_str),
                              MakeTypeTuple(py_tuple, py_str, py_str, py_bool)};
                 }
                 return py::frozenset(py::make_iterator(types.begin(), types.end()));
             }}};
    return type_map.at(type_index)();
}

}  // namespace python_bindings
