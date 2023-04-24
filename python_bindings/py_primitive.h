#pragma once

#include <unordered_map>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/algo_factory.h"
#include "dataframe_reader.h"
#include "parser/csv_parser.h"
#include "py_to_any.h"

namespace python_bindings {

template <typename PrimitiveType>
class PyPrimitive {
protected:
    void Configure(pybind11::kwargs const& kwargs);

    PrimitiveType primitive_;

public:
    template <typename... Args>
    PyPrimitive(Args&&... args) : primitive_(std::forward<Args>(args)...) {}

    // For pandas dataframes
    void Fit(pybind11::object dataframe, std::string name, pybind11::kwargs const& kwargs);

    void Fit(std::string const& path, char separator, bool has_header,
             pybind11::kwargs const& kwargs);

    pybind11::int_ Execute(pybind11::kwargs const& kwargs) {
        Configure(kwargs);
        return primitive_.Execute();
    }
};

template <typename PrimitiveType>
void PyPrimitive<PrimitiveType>::Configure(const pybind11::kwargs& kwargs) {
    static const auto void_index = std::type_index{typeid(void)};

    auto params = kwargs.cast<std::unordered_map<std::string, pybind11::object>>();
    std::unordered_map<std::string, boost::any> any_map{};
    for (auto const& [opt_name, obj] : params) {
        auto type_index = primitive_.GetTypeIndex(opt_name);
        if (type_index == void_index) continue;
        any_map[opt_name] = PyToAny(type_index, obj);
    }
    algos::ConfigureFromMap(primitive_, any_map);
}

template <typename PrimitiveType>
void PyPrimitive<PrimitiveType>::Fit(const std::string& path, char separator, bool has_header,
                                     const pybind11::kwargs& kwargs) {
    Configure(kwargs);
    CSVParser parser{path, separator, has_header};
    primitive_.Fit(parser);
}

template <typename PrimitiveType>
void PyPrimitive<PrimitiveType>::Fit(pybind11::object dataframe, std::string name,
                                     pybind11::kwargs const& kwargs) {
    Configure(kwargs);
    PandasReader reader{std::move(dataframe), std::move(name)};
    primitive_.Fit(reader);
}

}  // namespace python_bindings
