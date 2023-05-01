#include <functional>
#include <unordered_map>

#include <boost/any.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/ar_algorithm_enums.h"
#include "algorithms/metric/enums.h"

namespace python_bindings {
namespace py = pybind11;
using ConvFunc = std::function<boost::any(py::handle)>;

template <typename Type>
static std::pair<std::type_index, ConvFunc> const NormalConvPair{
        std::type_index(typeid(Type)), [](py::handle value) { return py::cast<Type>(value); }};

template <typename EnumType>
static std::pair<std::type_index, ConvFunc> const EnumConvPair{
        std::type_index(typeid(EnumType)), [](py::handle value) {
            return EnumType::_from_string_nocase(py::cast<std::string>(value).data());
        }};

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
};

boost::any PyToAny(std::type_index index, py::handle obj) {
    return converters.at(index)(obj);
}

}  // namespace python_bindings
