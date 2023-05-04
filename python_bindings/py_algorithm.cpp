#include "py_algorithm.h"

#include <string>
#include <unordered_map>

#include <boost/any.hpp>

#include "algorithms/algo_factory.h"
#include "algorithms/relational_algorithm.h"
#include "create_dataframe_reader.h"
#include "dataframe_reader.h"
#include "get_py_type.h"
#include "make_csv_parser.h"
#include "parser/csv_parser.h"
#include "py_to_any.h"
#include "util/config/names.h"

using algos::RelationStream;
using util::config::names::kData;

static const auto void_index = std::type_index{typeid(void)};

namespace python_bindings {

namespace py = pybind11;

void PyAlgorithmBase::Configure(py::kwargs const& kwargs,
                                std::function<boost::any()> const& create_data_reader) {
    algos::ConfigureFromFunction(*algorithm_, [this, &kwargs,
                                               create_data_reader](std::string_view option_name) {
        if (option_name == kData && create_data_reader) {
            return create_data_reader();
        }
        std::type_index type_index = algorithm_->GetTypeIndex(option_name);
        assert(type_index != void_index);
        return kwargs.contains(option_name) ? PyToAny(type_index, kwargs[py::str{option_name}])
                                            : boost::any{};
    });
}

void PyAlgorithmBase::SetOption(std::string_view option_name, py::handle option_value) {
    if (option_value.is_none()) {
        algorithm_->SetOption(option_name);
        return;
    }
    algorithm_->SetOption(option_name,
                          PyToAny(algorithm_->GetTypeIndex(option_name), option_value));
}

std::unordered_set<std::string_view> PyAlgorithmBase::GetNeededOptions() const {
    return algorithm_->GetNeededOptions();
}

py::frozenset PyAlgorithmBase::GetOptionType(std::string_view option_name) const {
    auto type_index = algorithm_->GetTypeIndex(option_name);
    if (type_index == void_index)
        throw std::invalid_argument{std::string{"Option named \""} + option_name.data() +
                                    "\" doesn't exist!"};
    return GetPyType(type_index);
}

void PyAlgorithmBase::LoadProvidedData(pybind11::kwargs const& kwargs,
                                       std::function<boost::any()> const& create_data_reader) {
    algorithm_->UnsetOption(kData);
    Configure(kwargs, create_data_reader);
    algorithm_->LoadData();
}

void PyAlgorithmBase::LoadData(std::string_view path, char separator, bool has_header,
                               py::kwargs const& kwargs) {
    LoadProvidedData(kwargs, [path, separator, has_header]() {
        return MakeCsvParser(path, separator, has_header);
    });
}

void PyAlgorithmBase::LoadData(std::string_view path, py::kwargs const& kwargs) {
    LoadProvidedData(kwargs, [path]() { return MakeCsvParser(path); });
}

void PyAlgorithmBase::LoadData(py::handle dataframe, std::string name, py::kwargs const& kwargs) {
    LoadProvidedData(kwargs, [dataframe, &name]() {
        return CreateDataFrameReader(dataframe, std::move(name));
    });
}

void PyAlgorithmBase::LoadData() {
    algorithm_->LoadData();
}

py::int_ PyAlgorithmBase::Execute(py::kwargs const& kwargs) {
    Configure(kwargs);
    return algorithm_->Execute();
}

}  // namespace python_bindings
