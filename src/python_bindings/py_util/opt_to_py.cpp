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
#include "core/util/enum_to_str.h"

namespace {
namespace py = pybind11;
using ConvFunction = std::function<py::object(boost::any)>;

template <typename T>
std::pair<std::type_index, ConvFunction> normal_conv_pair{
        std::type_index(typeid(T)),
        [](boost::any value) { return py::cast(boost::any_cast<T>(value)); }};

template <typename T>
std::pair<std::type_index, ConvFunction> enum_conv_pair{
        std::type_index(typeid(T)),
        [](boost::any value) { return py::cast(util::EnumToStr(boost::any_cast<T>(value))); }};
std::unordered_map<std::type_index, ConvFunction> const kConverters{
        normal_conv_pair<int>,
        normal_conv_pair<double>,
        normal_conv_pair<long double>,
        normal_conv_pair<unsigned int>,
        normal_conv_pair<bool>,
        normal_conv_pair<config::ThreadNumType>,
        normal_conv_pair<config::CustomRandomSeedType>,
        normal_conv_pair<config::MaxLhsType>,
        normal_conv_pair<config::ErrorType>,
        normal_conv_pair<config::IndicesType>,
        normal_conv_pair<model::DDString>,
        enum_conv_pair<algos::metric::MetricAlgo>,
        enum_conv_pair<algos::metric::Metric>,
        enum_conv_pair<algos::InputFormat>,
        enum_conv_pair<algos::hymd::LevelDefinition>,
        enum_conv_pair<algos::od::Ordering>};
}  // namespace

namespace python_bindings {
py::object OptToPy(std::type_index type, boost::any val) {
    return kConverters.at(type)(val);
}
}  // namespace python_bindings
