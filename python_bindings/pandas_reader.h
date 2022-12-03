#pragma once

#include <pybind11/pybind11.h>

#include "model/idataset_stream.h"

namespace python_bindings {

class PandasReader final : public model::IDatasetStream {
    pybind11::object dataframe_;
    std::string name_;
    std::vector<std::string> column_names_;
    pybind11::size_t row_ = 0;

public:
    PandasReader(pybind11::object dataframe, std::string name = "Pandas dataframe");

    void Reset() override;
    std::string GetRelationName() const override;
    std::string GetColumnName(int index) const override;
    size_t GetNumberOfColumns() const override;
    bool HasNextRow() const override;
    std::vector<std::string> GetNextRow() override;
};

}  // namespace python_bindings
