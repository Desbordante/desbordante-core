#pragma once

#include <pybind11/pybind11.h>

#include "model/idataset_stream.h"

namespace python_bindings {

class DataframeReaderBase : public model::IDatasetStream {
protected:
    pybind11::object dataframe_;
    pybind11::iterator df_iter_;
    std::string name_;
    std::vector<std::string> column_names_;

public:
    explicit DataframeReaderBase(pybind11::object dataframe, std::string name = "Pandas dataframe");

    void Reset() final;
    [[nodiscard]] std::string GetRelationName() const final;
    [[nodiscard]] std::string GetColumnName(int index) const final;
    [[nodiscard]] size_t GetNumberOfColumns() const final;
    [[nodiscard]] bool HasNextRow() const final;
};

class StringDataframeReader final : public DataframeReaderBase {
public:
    using DataframeReaderBase::DataframeReaderBase;

    std::vector<std::string> GetNextRow() final;
};

class ArbitraryDataframeReader final : public DataframeReaderBase {
private:
    // Used in GetNextRow to check arbitrary dataframe values for nullity.
    // Statically storing this may cause problems with garbage collection, but
    // importing a module on every call may cause performance problems, and
    // storing it as a field is a compromise, which is used here.
    std::function<bool(pybind11::handle)> is_null_ =
            [check_null = pybind11::module_::import("pandas").attr("isna")](pybind11::handle obj) {
                return check_null(obj).cast<bool>();
            };

public:
    using DataframeReaderBase::DataframeReaderBase;

    [[nodiscard]] std::vector<std::string> GetNextRow() final;
};

}  // namespace python_bindings
