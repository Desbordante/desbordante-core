#include "python_bindings/py_util/opt_to_py.h"

#include <functional>
#include <typeinfo>
#include <unordered_map>

#include <pybind11/stl.h>

#include "core/algorithms/association_rules/ar_algorithm_enums.h"
#include "core/algorithms/dd/dd.h"
#include "core/algorithms/md/hymd/enums.h"
#include "core/algorithms/metric/enums.h"
#include "core/algorithms/od/fastod/od_ordering.h"
#include "core/config/custom_random_seed/type.h"
#include "core/config/equal_nulls/type.h"
#include "core/config/error/type.h"
#include "core/config/indices/type.h"
#include "core/config/max_lhs/type.h"
#include "core/config/thread_number/type.h"

namespace {
namespace py = pybind11;
using ConvFunction = std::function<py::object(boost::any)>;

template <typename T>
std::pair<std::type_index, ConvFunction> const kNormalConvPair{
        std::type_index(typeid(T)),
        [](boost::any value) { return py::cast(boost::any_cast<T>(value)); }};

template <typename T>
std::pair<std::type_index, ConvFunction> const kEnumConvPair{
        std::type_index(typeid(T)),
        [](boost::any value) { return py::cast(boost::any_cast<T>(value)._to_string()); }};

std::unordered_map<std::type_index, ConvFunction> const kConverters{
        kNormalConvPair<int>,
        kNormalConvPair<double>,
        kNormalConvPair<long double>,
        kNormalConvPair<unsigned int>,
        kNormalConvPair<bool>,
        kNormalConvPair<config::ThreadNumType>,
        kNormalConvPair<config::CustomRandomSeedType>,
        kNormalConvPair<config::MaxLhsType>,
        kNormalConvPair<config::ErrorType>,
        kNormalConvPair<config::IndicesType>,
        kNormalConvPair<model::DDString>,
        kEnumConvPair<algos::metric::MetricAlgo>,
        kEnumConvPair<algos::metric::Metric>,
        kEnumConvPair<algos::InputFormat>,
        kEnumConvPair<algos::hymd::LevelDefinition>,
        kEnumConvPair<algos::od::Ordering>};
}  // namespace

namespace python_bindings {
py::object OptToPy(std::type_index type, boost::any val) {
    return kConverters.at(type)(val);
}
}  // namespace python_bindings
