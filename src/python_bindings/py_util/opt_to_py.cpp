#include "opt_to_py.h"

#include <functional>
#include <typeinfo>
#include <unordered_map>

#include <pybind11/stl.h>

#include "algorithms/dd/dd.h"
#include "algorithms/md/hymd/enums.h"
#include "algorithms/metric/enums.h"
#include "algorithms/pac/model/default_domains/domain_type.h"
#include "algorithms/pac/model/idomain.h"
#include "association_rules/ar_algorithm_enums.h"
#include "config/custom_random_seed/type.h"
#include "config/equal_nulls/type.h"
#include "config/error/type.h"
#include "config/indices/type.h"
#include "config/max_lhs/type.h"
#include "config/thread_number/type.h"

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
        [](boost::any value) { return py::cast(boost::any_cast<T>(value)._to_string()); }};
std::unordered_map<std::type_index, ConvFunction> const kConverters{
        normal_conv_pair<int>,
        normal_conv_pair<double>,
        normal_conv_pair<long double>,
        normal_conv_pair<unsigned int>,
        normal_conv_pair<bool>,
        normal_conv_pair<std::string>,
        normal_conv_pair<config::ThreadNumType>,
        normal_conv_pair<config::CustomRandomSeedType>,
        normal_conv_pair<config::MaxLhsType>,
        normal_conv_pair<config::ErrorType>,
        normal_conv_pair<config::IndicesType>,
        normal_conv_pair<model::DDString>,
        normal_conv_pair<std::vector<std::string>>,
        normal_conv_pair<std::vector<double>>,
        normal_conv_pair<std::shared_ptr<pac::model::IDomain>>,
        enum_conv_pair<algos::metric::MetricAlgo>,
        enum_conv_pair<algos::metric::Metric>,
        enum_conv_pair<algos::InputFormat>,
        enum_conv_pair<algos::hymd::LevelDefinition>,
        enum_conv_pair<pac::model::DomainType>,
};
}  // namespace

namespace python_bindings {
py::object OptToPy(std::type_index type, boost::any val) {
    return kConverters.at(type)(val);
}
}  // namespace python_bindings
