#pragma once

#include <memory>
#include <type_traits>
#include <unordered_map>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/algorithm.h"
#include "util/config/tabular_data/input_table_type.h"

namespace python_bindings {

class PyAlgorithmBase {
private:
    void LoadProvidedData(pybind11::kwargs const& kwargs, util::config::InputTable table);

protected:
    std::unique_ptr<algos::Algorithm> algorithm_;

    explicit PyAlgorithmBase(std::unique_ptr<algos::Algorithm> ptr) : algorithm_(std::move(ptr)) {}

    void Configure(pybind11::kwargs const& kwargs, util::config::InputTable table = nullptr);

public:
    void SetOption(std::string_view option_name, pybind11::handle option_value);

    [[nodiscard]] std::unordered_set<std::string_view> GetNeededOptions() const;

    [[nodiscard]] pybind11::frozenset GetOptionType(std::string_view option_name) const;

    // For pandas dataframes
    void LoadData(pybind11::handle dataframe, std::string name, pybind11::kwargs const& kwargs);

    void LoadData(std::string_view path, char separator, bool has_header,
                  pybind11::kwargs const& kwargs);

    // For the case when the "data" option has been set by `set_option`.
    // Currently, there is no use case where we want to set other options in
    // bulk with kwargs when the "data" option is set by `set_option`, so this
    // method assumes that all the other data loading options have already been
    // set by `set_option` as well and doesn't accept any arguments. This only
    // moves the algorithm to the execution stage.
    void LoadData();

    pybind11::int_ Execute(pybind11::kwargs const& kwargs);
};

template <typename AlgorithmType, typename Base>
class PyAlgorithm : public Base {
    static_assert(std::is_base_of_v<PyAlgorithmBase, Base>);

public:
    PyAlgorithm() : Base(std::make_unique<AlgorithmType>()) {}
};

}  // namespace python_bindings
