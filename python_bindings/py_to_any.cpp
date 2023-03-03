#include <functional>
#include <unordered_map>

#include <boost/any.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/ar_algorithm_enums.h"
#include "algorithms/metric/enums.h"

#define NORMAL_CONV(type)                         \
    std::make_pair(std::type_index(typeid(type)), \
                   [](py::object const& value) { return py::cast<type>(value); })

#define ENUM_CONV(enum_type)                                                         \
    std::make_pair(std::type_index(typeid(enum_type)), [](py::object const& value) { \
        return enum_type::_from_string_nocase(py::cast<std::string>(value).data());  \
    })

namespace python_bindings {
namespace py = pybind11;
using ConvFunc = std::function<boost::any(py::object const&)>;

static const std::unordered_map<std::type_index, ConvFunc> converters{
        NORMAL_CONV(bool),
        NORMAL_CONV(double),
        NORMAL_CONV(unsigned int),
        NORMAL_CONV(bool),
        NORMAL_CONV(long double),
        NORMAL_CONV(std::vector<unsigned int>),
        NORMAL_CONV(ushort),
        NORMAL_CONV(int),
        ENUM_CONV(algos::metric::Metric),
        ENUM_CONV(algos::metric::MetricAlgo),
        ENUM_CONV(algos::InputFormat),
};
#undef ENUM_CONV
#undef NORMAL_CONV

boost::any PyToAny(std::type_index index, py::object const& obj) {
    return converters.at(index)(obj);
}

}  // namespace python_bindings
