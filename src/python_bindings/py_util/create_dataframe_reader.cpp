#include "python_bindings/py_util/create_dataframe_reader.h"

#include "core/config/exceptions.h"
#include "python_bindings/py_util/dataframe_reader.h"

namespace python_bindings {

namespace py = pybind11;

static bool IsDataFrame(py::handle object) {
    try {
        return py::isinstance(object, py::module::import("pandas").attr("DataFrame"));
    } catch (py::error_already_set& e) {
        if (e.matches(PyExc_ImportError)) {
            return false;
        } else {
            throw;
        }
    }
}

static bool AllColumnsAreStrings(py::handle dataframe) {
    py::object dtypes = dataframe.attr("dtypes");
    return dtypes[dtypes.attr("__ne__")(py::str{"string"})].attr("empty").cast<bool>();
}

config::InputTable CreateDataFrameReader(py::handle dataframe) {
    if (!IsDataFrame(dataframe))
        throw config::ConfigurationError("Passed object is not a dataframe");
    std::string name = [&]() -> std::string {
        try {
            return py::str(dataframe.attr("attrs")["name"]);
        } catch (py::error_already_set& e) {
            if (!(e.matches(PyExc_KeyError) || e.matches(PyExc_AttributeError))) throw;
            return "Pandas dataframe";
        };
    }();
    if (AllColumnsAreStrings(dataframe)) {
        return std::make_shared<StringDataframeReader>(dataframe, std::move(name));
    } else {
        return std::make_shared<ArbitraryDataframeReader>(dataframe, std::move(name));
    }
}

}  // namespace python_bindings
