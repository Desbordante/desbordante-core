#pragma once

#include <memory>
#include <type_traits>
#include <unordered_map>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/algorithm.h"
#include "config/names.h"
#include "config/tabular_data/input_table_type.h"
#include "opt_to_py.h"

namespace python_bindings {

class PyAlgorithmBase {
private:
    void LoadProvidedData(pybind11::kwargs const& kwargs, config::InputTable table);

protected:
    std::unique_ptr<algos::Algorithm> algorithm_;

    explicit PyAlgorithmBase(std::unique_ptr<algos::Algorithm> ptr) : algorithm_(std::move(ptr)) {}

    void Configure(pybind11::kwargs const& kwargs, config::InputTable table = nullptr);

public:
    void SetOption(std::string_view option_name, pybind11::handle option_value);

    [[nodiscard]] std::unordered_set<std::string_view> GetNeededOptions() const;

    [[nodiscard]] std::unordered_set<std::string_view> GetPossibleOptions() const;

    [[nodiscard]] std::string_view GetDescription(std::string_view option_name) const;
    [[nodiscard]] pybind11::tuple GetOptionType(std::string_view option_name) const;

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

    std::unordered_map<std::string, pybind11::object> GetOpts() {
        auto opt_value_info = algorithm_->GetOptValues();
        std::unordered_map<std::string, pybind11::object> res;
        for (auto const& [name, value_info] : opt_value_info) {
            if (name == config::names::kTable) {
                continue;
            }
            res[std::string(name)] = OptToPy(value_info.type, value_info.value);
        }
        return res;
    }
};

template <typename AlgorithmType, typename Base>
class PyAlgorithm : public Base {
    static_assert(std::is_base_of_v<PyAlgorithmBase, Base>);

public:
    PyAlgorithm() : Base(std::make_unique<AlgorithmType>()) {}
};

}  // namespace python_bindings
