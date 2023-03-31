#include "pandas_reader.h"

#include <string>
#include <utility>
#include <vector>

#include <Python.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

namespace python_bindings {

namespace py = pybind11;

static std::vector<std::string> GetColumnNames(pybind11::object const& dataframe) {
    std::vector<std::string> names;
    py::list name_lst = dataframe.attr("columns").attr("to_list")();
    for (py::handle element : name_lst) {
        names.emplace_back(py::str(element));
    }
    return names;
}

PandasReaderBase::PandasReaderBase(pybind11::object dataframe, std::string name)
    : dataframe_(std::move(dataframe)),
      df_iter_(dataframe_.attr("itertuples")(false, py::none{})),
      name_(std::move(name)),
      column_names_(GetColumnNames(dataframe_)) {}

void PandasReaderBase::Reset() {
    df_iter_ = dataframe_.attr("itertuples")(false, py::none{});
}

std::string PandasReaderBase::GetRelationName() const {
    return name_;
}

std::string PandasReaderBase::GetColumnName(int index) const {
    return column_names_.at(index);
}

size_t PandasReaderBase::GetNumberOfColumns() const {
    return column_names_.size();
}

bool PandasReaderBase::HasNextRow() const {
    return df_iter_ != py::iterator::sentinel();
}

template <>
std::vector<std::string> PandasReader<false>::GetNextRow() {
    return py::cast<std::vector<std::string>>(*df_iter_++);
}

template <>
std::vector<std::string> PandasReader<true>::GetNextRow() {
    std::vector<std::string> strings{};
    auto list = *df_iter_++.cast<py::list>();
    for (py::handle el : list) {
        // When reading from .csv files, pandas treats some values as nulls,
        // and empty values are among those values, which may cause some
        // confusion here, since in Desbordante only the literal "NULL" string
        // is interpreted as the null value.
        if (py::isinstance<py::float_>(el) && std::isnan(el.cast<double>())) {
            strings.emplace_back("NULL");
        } else {
            strings.emplace_back(pybind11::str(el));
        }
    }
    return strings;
}

}  // namespace python_bindings
