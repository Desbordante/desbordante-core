#include "python_bindings/py_util/opt_to_py.h"

#include <functional>
#include <sstream>
#include <stdexcept>
#include <typeinfo>
#include <unordered_map>

#include <boost/core/demangle.hpp>
#include <pybind11/stl.h>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/md/hymd/enums.h"
#include "core/algorithms/metric/enums.h"
#include "core/algorithms/nar/des/enums.h"
#include "core/algorithms/od/fastod/od_ordering.h"
#include "core/config/custom_metric/custom_metric/type.h"
#include "core/config/custom_metric/custom_metrics/type.h"
#include "core/config/custom_metric/custom_vector_metric/type.h"
#include "core/config/custom_random_seed/type.h"
#include "core/config/equal_nulls/type.h"
#include "core/config/error/type.h"
#include "core/config/indices/type.h"
#include "core/config/max_lhs/type.h"
#include "core/config/thread_number/type.h"
#include "core/model/transaction/input_format_type.h"
#include "core/util/enum_to_str.h"

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
        [](boost::any value) { return py::cast(util::EnumToStr(boost::any_cast<T>(value))); }};

std::unordered_map<std::type_index, ConvFunction> const kConverters{
        kNormalConvPair<int>,
        kNormalConvPair<double>,
        kNormalConvPair<long double>,
        kNormalConvPair<unsigned int>,
        kNormalConvPair<bool>,
        kNormalConvPair<std::vector<std::string>>,
        kNormalConvPair<config::ThreadNumType>,
        kNormalConvPair<config::CustomRandomSeedType>,
        kNormalConvPair<config::MaxLhsType>,
        kNormalConvPair<config::ErrorType>,
        kNormalConvPair<config::IndicesType>,
        kNormalConvPair<model::DDString>,
        kNormalConvPair<model::Gdd>,
        kNormalConvPair<config::CustomMetricType>,
        kNormalConvPair<config::CustomMetricsType>,
        kNormalConvPair<config::CustomVectorMetricType>,
        kEnumConvPair<algos::metric::MetricAlgo>,
        kEnumConvPair<algos::metric::Metric>,
        kEnumConvPair<model::InputFormatType>,
        kEnumConvPair<algos::hymd::LevelDefinition>,
        kEnumConvPair<algos::des::DifferentialStrategy>,
        kEnumConvPair<algos::od::Ordering>};
}  // namespace

namespace python_bindings {
py::object OptToPy(std::type_index type, boost::any val) {
    auto const it = kConverters.find(type);
    if (it == kConverters.end()) [[unlikely]] {
        std::ostringstream oss;
        oss << "Unknown option type: " << boost::core::demangle(type.name()) << " (OptToPy)";
        throw std::runtime_error(oss.str());
    }
    return it->second(val);
}
}  // namespace python_bindings
