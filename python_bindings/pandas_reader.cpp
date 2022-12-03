#include "pandas_reader.h"

#include <string>
#include <utility>
#include <vector>

#include <pybind11/pybind11.h>

namespace python_bindings {

namespace py = pybind11;

static std::vector<std::string> GetColumnNames(pybind11::object const& dataframe) {
    std::vector<std::string> names;
    py::list name_lst = dataframe.attr("columns").attr("to_list")();
    for (auto element : name_lst) {
        names.emplace_back(py::str(element));
    }
    return names;
}

PandasReader::PandasReader(pybind11::object dataframe, std::string name)
    : dataframe_(std::move(dataframe)),
      name_(std::move(name)),
      column_names_(GetColumnNames(dataframe_)) {}

void PandasReader::Reset() {
    row_ = 0;
}

std::string PandasReader::GetRelationName() const {
    return name_;
}

std::string PandasReader::GetColumnName(int index) const {
    return column_names_.at(index);
}

size_t PandasReader::GetNumberOfColumns() const {
    return column_names_.size();
}

bool PandasReader::HasNextRow() const {
    return row_ < pybind11::len(dataframe_);
}

std::vector<std::string> PandasReader::GetNextRow() {
    std::vector<std::string> strings{};
    auto list = pybind11::list(dataframe_.attr("iloc")[pybind11::int_(row_)]);
    for (auto el : list) {
        strings.emplace_back(pybind11::str(el));
    }
    ++row_;
    return strings;
}

}  // namespace python_bindings
