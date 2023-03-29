#pragma once

#include <pybind11/pybind11.h>

#include "model/idataset_stream.h"

namespace python_bindings {

class PandasReaderBase : public model::IDatasetStream {
protected:
    pybind11::object dataframe_;
    pybind11::iterator df_iter_;
    std::string name_;
    std::vector<std::string> column_names_;

public:
    PandasReaderBase(pybind11::object dataframe, std::string name = "Pandas dataframe");

    void Reset() final;
    [[nodiscard]] std::string GetRelationName() const final;
    [[nodiscard]] std::string GetColumnName(int index) const final;
    [[nodiscard]] size_t GetNumberOfColumns() const final;
    [[nodiscard]] bool HasNextRow() const final;
};

template <bool transform_to_string>
class PandasReader final : public PandasReaderBase {
public:
    using PandasReaderBase::PandasReaderBase;

    [[nodiscard]] std::vector<std::string> GetNextRow() final;
};

}  // namespace python_bindings
