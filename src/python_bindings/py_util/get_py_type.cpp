#include "python_bindings/py_util/get_py_type.h"

#include <functional>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <Python.h>
#include <pybind11/stl/filesystem.h>

#include "core/algorithms/association_rules/ar_algorithm_enums.h"
#include "core/algorithms/cfd/enums.h"
#include "core/algorithms/dd/dd.h"
#include "core/algorithms/md/hymd/enums.h"
#include "core/algorithms/md/hymd/hymd.h"
#include "core/algorithms/md/md_verifier/column_similarity_classifier.h"
#include "core/algorithms/metric/enums.h"
#include "core/algorithms/od/fastod/od_ordering.h"
#include "core/config/custom_random_seed/type.h"
#include "core/config/error_measure/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/config/tabular_data/input_tables_type.h"
#include "core/model/table/column_combination.h"

namespace py = pybind11;

namespace {

py::handle MakeType(py::type type) {
    return type;
}

py::handle MakeType(PyTypeObject* py_type_ptr) {
    // Does the same as `&py_type_ptr->ob_base.ob_base`: Python API simulates
    // inheritance here. There is also the `_PyObject_CAST` macro that does a
    // C-style cast, but, as indicated by the underscore, it's an internal
    // Python macro, so it should be avoided.
    return reinterpret_cast<PyObject*>(py_type_ptr);
}

[[maybe_unused]] py::handle MakeType(PyObject* py_type_ptr) {
    return py_type_ptr;
}

template <typename... TypePtrs>
py::tuple MakeTypeTuple(TypePtrs... type_ptrs) {
    return py::make_tuple(MakeType(type_ptrs)...);
}

template <typename CppType, PyTypeObject*... PyTypes>
std::pair<std::type_index, std::function<py::tuple()>> const kPyTypePair{
        std::type_index{typeid(CppType)}, []() { return MakeTypeTuple(PyTypes...); }};

}  // namespace

namespace python_bindings {

py::tuple GetPyType(std::type_index type_index) {
    // Type indexes are mapped to Python type tuples. The first element of all
    // the tuples is the type of the parameter itself. If that element is
    // `list`, the following element denotes the type of its elements. If that
    // parameter is `tuple` then the following parameters denote the types of
    // their respective elements.
    // The tuples are created at runtime from Python API's raw pointers (when
    // possible) as storing pybind11's objects themselves statically is
    // unpredictable and can lead to errors related to garbage collection.
    static std::unordered_map<std::type_index, std::function<py::tuple()>> const type_map{
            kPyTypePair<bool, &PyBool_Type>,
            kPyTypePair<unsigned short, &PyLong_Type>,
            kPyTypePair<int, &PyLong_Type>,
            kPyTypePair<unsigned int, &PyLong_Type>,
            kPyTypePair<double, &PyFloat_Type>,
            kPyTypePair<size_t, &PyLong_Type>,
            kPyTypePair<long double, &PyFloat_Type>,
            kPyTypePair<std::size_t, &PyLong_Type>,
            kPyTypePair<config::CustomRandomSeedType, &PyLong_Type>,
            kPyTypePair<algos::metric::Metric, &PyUnicode_Type>,
            kPyTypePair<algos::metric::MetricAlgo, &PyUnicode_Type>,
            kPyTypePair<config::PfdErrorMeasureType, &PyUnicode_Type>,
            kPyTypePair<config::AfdErrorMeasureType, &PyUnicode_Type>,
            kPyTypePair<algos::InputFormat, &PyUnicode_Type>,
            kPyTypePair<algos::cfd::Substrategy, &PyUnicode_Type>,
            kPyTypePair<algos::hymd::LevelDefinition, &PyUnicode_Type>,
            kPyTypePair<algos::od::Ordering, &PyUnicode_Type>,
            kPyTypePair<std::vector<unsigned int>, &PyList_Type, &PyLong_Type>,
            {typeid(algos::hymd::HyMD::ColumnMatches),
             []() {
                 return MakeTypeTuple(
                         &PyList_Type,
                         py::type::of<algos::hymd::preprocessing::column_matches::ColumnMatch>());
             }},
            {typeid(model::DDString),
             []() {
                 return MakeTypeTuple(&PyTuple_Type, &PyList_Type,
                                      py::type::of<model::DFStringConstraint>());
             }},
            {typeid(config::InputTable),
             []() { return MakeTypeTuple(py::type::of<config::InputTable>()); }},
            {typeid(config::InputTables),
             []() { return MakeTypeTuple(&PyList_Type, py::type::of<config::InputTable>()); }},
            {typeid(algos::md::ColumnSimilarityClassifier),
             []() { return MakeTypeTuple(py::type::of<algos::md::ColumnSimilarityClassifier>()); }},
            {typeid(std::vector<algos::md::ColumnSimilarityClassifier>),
             []() {
                 return MakeTypeTuple(&PyList_Type,
                                      py::type::of<algos::md::ColumnSimilarityClassifier>());
             }},
            kPyTypePair<std::filesystem::path, &PyUnicode_Type>,
            kPyTypePair<std::vector<std::filesystem::path>, &PyList_Type, &PyUnicode_Type>,
            kPyTypePair<std::unordered_set<size_t>, &PySet_Type, &PyLong_Type>,
            kPyTypePair<std::string, &PyUnicode_Type>,
    };
    return type_map.at(type_index)();
}

}  // namespace python_bindings
