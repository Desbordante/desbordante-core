#include <functional>
#include <unordered_map>

#include <boost/any.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/ar_algorithm_enums.h"
#include "algorithms/metric/enums.h"
#include "create_dataframe_reader.h"
#include "csv_parser.h"
#include "util/config/tabular_data/input_table_type.h"

namespace python_bindings {
namespace py = pybind11;
using ConvFunc = std::function<boost::any(py::handle)>;

static util::config::InputTable CreateCsvParser(py::tuple const& arguments) {
    if (py::len(arguments) != 3) {
        throw py::value_error("Cannot create a csv parser from passed tuple.");
    }
    return std::make_shared<CSVParser>(py::cast<std::string>(arguments[0]),
                                       py::cast<char>(arguments[1]), py::cast<bool>(arguments[2]));
}

template <typename Type>
static std::pair<std::type_index, ConvFunc> const NormalConvPair{
        std::type_index(typeid(Type)), [](py::handle value) { return py::cast<Type>(value); }};

template <typename EnumType>
static std::pair<std::type_index, ConvFunc> const EnumConvPair{
        std::type_index(typeid(EnumType)), [](py::handle value) {
            return EnumType::_from_string_nocase(py::cast<std::string>(value).data());
        }};

static boost::any InputTableToAny(py::handle obj) {
    if (py::isinstance<py::tuple>(obj)) {
        return CreateCsvParser(py::cast<py::tuple>(obj));
    }
    return CreateDataFrameReader(obj);
}

static const std::unordered_map<std::type_index, ConvFunc> converters{
        NormalConvPair<bool>,
        NormalConvPair<double>,
        NormalConvPair<unsigned int>,
        NormalConvPair<long double>,
        NormalConvPair<std::vector<unsigned int>>,
        NormalConvPair<ushort>,
        NormalConvPair<int>,
        EnumConvPair<algos::metric::Metric>,
        EnumConvPair<algos::metric::MetricAlgo>,
        EnumConvPair<algos::InputFormat>,
        {typeid(util::config::InputTable), InputTableToAny},
};

boost::any PyToAny(std::type_index index, py::handle obj) {
    return converters.at(index)(obj);
}

}  // namespace python_bindings
