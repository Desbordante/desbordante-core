// clang-format off
#include <pybind11/pybind11.h>
// clang-format on

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>

#include <boost/any.hpp>
#include <boost/core/demangle.hpp>
#include <pybind11/functional.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>
#include <pybind11/typing.h>
#include <pybind11/cast.h>

#include "core/algorithms/algebraic_constraints/bin_operation_enum.h"
#include "core/algorithms/association_rules/ar_algorithm_enums.h"
#include "core/algorithms/cfd/enums.h"
#include "core/algorithms/dd/dd.h"
#include "core/algorithms/fd/afd_metric/afd_metric.h"
#include "core/algorithms/md/hymd/enums.h"
#include "core/algorithms/md/hymd/hymd.h"
#include "core/algorithms/md/md_verifier/column_similarity_classifier.h"
#include "core/algorithms/metric/enums.h"
#include "core/algorithms/od/fastod/od_ordering.h"
#include "core/algorithms/pac/model/default_domains/domain_type.h"
#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/column_metric.h"
#include "core/config/custom_random_seed/type.h"
#include "core/config/error_measure/type.h"
#include "core/config/exceptions.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/config/tabular_data/input_tables_type.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "core/util/enum_to_available_values.h"
#include "python_bindings/py_util/create_dataframe_reader.h"
#include "core/util/logger.h"

namespace {

namespace py = pybind11;
using ConvFunc = std::function<boost::any(std::string_view, py::handle)>;

template <typename T>
T CastAndReplaceCastError(std::string_view option_name, py::handle value) {
    try {
        return py::cast<T>(value);
    } catch (py::cast_error& e) {
        throw config::ConfigurationError(
                std::string("Unable to cast Python object to C++ type for option \"") +
                option_name.data() + '"');
    }
}

config::InputTable CreateCsvParser(std::string_view option_name, py::tuple const& arguments) {
    if (py::len(arguments) != 3) {
        throw config::ConfigurationError("Cannot create a CSV parser from passed tuple.");
    }

    return std::make_shared<CSVParser>(
            CastAndReplaceCastError<std::string>(option_name, arguments[0]),
            CastAndReplaceCastError<char>(option_name, arguments[1]),
            CastAndReplaceCastError<bool>(option_name, arguments[2]));
}

config::InputTable PythonObjToInputTable(std::string_view option_name, py::handle obj) {
    if (py::isinstance<py::tuple>(obj)) {
        return CreateCsvParser(option_name, py::cast<py::tuple>(obj));
    }
    return python_bindings::CreateDataFrameReader(obj);
}

template <typename Type>
std::pair<std::type_index, ConvFunc> const kNormalConvPair{
        std::type_index(typeid(Type)), [](std::string_view option_name, py::handle value) {
            return CastAndReplaceCastError<Type>(option_name, value);
        }};

template <typename EnumType>
std::pair<std::type_index, ConvFunc> const kEnumConvPair{
        std::type_index(typeid(EnumType)), [](std::string_view option_name, py::handle value) {
            auto string = CastAndReplaceCastError<std::string>(option_name, value);
            better_enums::optional<EnumType> enum_holder =
                    EnumType::_from_string_nocase_nothrow(string.data());
            if (enum_holder) return *enum_holder;

            std::stringstream error_message;
            error_message << "Incorrect value for option \"" << option_name
                          << "\". Possible values: " << util::EnumToAvailableValues<EnumType>();
            throw config::ConfigurationError(error_message.str());
        }};

template <typename EnumType>
std::pair<std::type_index, ConvFunc> const kCharEnumConvPair{
        std::type_index(typeid(EnumType)), [](std::string_view option_name, py::handle value) {
            using EnumValueType = typename EnumType::_integral;
            // May be applicable to other types.
            static_assert(std::is_same_v<EnumValueType, char>);
            auto char_value = CastAndReplaceCastError<char>(option_name, value);
            better_enums::optional<EnumType> enum_holder =
                    EnumType::_from_integral_nothrow(char_value);
            if (enum_holder) return *enum_holder;

            std::stringstream error_message;
            error_message << "Incorrect value for option \"" << option_name
                          << "\". Possible values: ";

            error_message << '[';
            for (auto const& val : EnumType::_values()) {
                error_message << val._to_integral() << '|';
            }
            error_message.seekp(-1, std::stringstream::cur);
            error_message << ']';

            throw config::ConfigurationError(error_message.str());
        }};

boost::any InputTableToAny(std::string_view option_name, py::handle obj) {
    return PythonObjToInputTable(option_name, obj);
}

boost::any InputTablesToAny(std::string_view option_name, py::handle obj) {
    auto tables = CastAndReplaceCastError<std::vector<py::handle>>(option_name, obj);
    std::vector<config::InputTable> parsers;
    for (auto const& table : tables) parsers.push_back(PythonObjToInputTable(option_name, table));
    return parsers;
}

boost::any FDPACVerifierValueMetricsToAny(std::string_view option_name, py::handle obj) {
	using namespace algos::pac_verifier;

    auto metric_handles = CastAndReplaceCastError<std::vector<py::handle>>(option_name, obj);
    std::vector<ValueMetric> metrics;
    metrics.reserve(metric_handles.size());
    for (auto const& handle : metric_handles) {
		LOG_INFO("Processing handle...");
        if (handle.is_none()) {
			LOG_INFO("None");
            metric_handles.push_back(nullptr);
        } else if (py::isinstance<py::typing::Callable<double(std::string, std::string)>>(handle)) {
			LOG_INFO("Function");
            metrics.emplace_back(py::cast<StringDataMetric>(handle));
        } else {
            throw config::ConfigurationError(
                    "FD PAC Verfier's Value metric must be either None or Callable[[str, str], "
                    "float]");
        }
    }
    return metrics;
}

std::unordered_map<std::type_index, ConvFunc> const kConverters{
        kNormalConvPair<bool>,
        kNormalConvPair<double>,
        kNormalConvPair<unsigned int>,
        kNormalConvPair<long double>,
        kNormalConvPair<std::vector<unsigned int>>,
        kNormalConvPair<unsigned short>,
        kNormalConvPair<int>,
        kNormalConvPair<size_t>,
        kNormalConvPair<algos::hymd::HyMD::ColumnMatches>,
        kNormalConvPair<std::optional<int>>,
        kNormalConvPair<algos::md::ColumnSimilarityClassifier>,
        kNormalConvPair<std::vector<algos::md::ColumnSimilarityClassifier>>,
        kEnumConvPair<algos::metric::Metric>,
        kEnumConvPair<algos::metric::MetricAlgo>,
        kEnumConvPair<config::PfdErrorMeasureType>,
        kEnumConvPair<config::AfdErrorMeasureType>,
        kEnumConvPair<algos::afd_metric_calculator::AFDMetric>,
        kEnumConvPair<algos::InputFormat>,
        kEnumConvPair<algos::cfd::Substrategy>,
        kEnumConvPair<algos::hymd::LevelDefinition>,
        kEnumConvPair<algos::od::Ordering>,
        kCharEnumConvPair<algos::Binop>,
        {typeid(config::InputTable), InputTableToAny},
        {typeid(config::InputTables), InputTablesToAny},
        kNormalConvPair<std::filesystem::path>,
        kNormalConvPair<std::vector<std::filesystem::path>>,
        kNormalConvPair<std::unordered_set<size_t>>,
        kNormalConvPair<model::DDString>,
        kNormalConvPair<std::string>,
        kNormalConvPair<std::vector<std::pair<std::string, std::string>>>,
        kNormalConvPair<std::pair<std::string, std::string>>,
        kEnumConvPair<pac::model::DomainType>,
        kNormalConvPair<std::vector<std::string>>,
        kNormalConvPair<std::shared_ptr<pac::model::IDomain>>,
        kNormalConvPair<std::vector<double>>,
        {typeid(std::vector<algos::pac_verifier::ValueMetric>), FDPACVerifierValueMetricsToAny},
};

}  // namespace

namespace python_bindings {

boost::any PyToAny(std::string_view option_name, std::type_index index, py::handle obj) {
    auto const it = kConverters.find(index);
    if (it == kConverters.end()) {
        std::ostringstream oss;
        oss << "Cannot get type for option " << option_name << ": "
            << boost::core::demangle(index.name()) << " (PyToAny)";
        throw std::runtime_error(oss.str());
    }
    return it->second(option_name, obj);
}

}  // namespace python_bindings
