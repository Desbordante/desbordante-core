#include "py_algorithm.h"

#include <string>
#include <unordered_map>

#include <boost/any.hpp>

#include "algorithms/algo_factory.h"
#include "pandas_reader.h"
#include "parser/csv_parser.h"
#include "py_to_any.h"

namespace python_bindings {

namespace py = pybind11;

void PyAlgorithmBase::Configure(py::kwargs const& kwargs) {
    static const auto void_index = std::type_index{typeid(void)};

    auto params = kwargs.cast<std::unordered_map<std::string, py::object>>();
    std::unordered_map<std::string, boost::any> any_map{};
    for (auto const& [opt_name, obj] : params) {
        std::type_index type_index = algorithm_->GetTypeIndex(opt_name);
        if (type_index == void_index) continue;
        any_map[opt_name] = PyToAny(type_index, obj);
    }
    algos::ConfigureFromMap(*algorithm_, any_map);
}

void PyAlgorithmBase::Fit(std::string const& path, char separator, bool has_header,
                          py::kwargs const& kwargs) {
    Configure(kwargs);
    CSVParser parser{path, separator, has_header};
    algorithm_->Fit(parser);
}

void PyAlgorithmBase::Fit(py::object dataframe, std::string name, py::kwargs const& kwargs) {
    Configure(kwargs);

    py::handle const& dtypes = dataframe.attr("dtypes");
    if (dtypes[dtypes.attr("__ne__")(py::str{"string"})].attr("empty").cast<bool>()) {
        // All columns are python strings, no need to transform.
        PandasReader<false> reader{std::move(dataframe), std::move(name)};
        algorithm_->Fit(reader);
    } else {
        PandasReader<true> reader{std::move(dataframe), std::move(name)};
        algorithm_->Fit(reader);
    }
}

py::int_ PyAlgorithmBase::Execute(py::kwargs const& kwargs) {
    Configure(kwargs);
    return algorithm_->Execute();
}

}  // namespace python_bindings
