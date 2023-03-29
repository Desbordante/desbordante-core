#pragma once

#include <memory>
#include <type_traits>
#include <unordered_map>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/primitive.h"

namespace python_bindings {

class PyAlgorithmBase {
protected:
    std::unique_ptr<algos::Primitive> algorithm_;

    explicit PyAlgorithmBase(std::unique_ptr<algos::Primitive> ptr) : algorithm_(std::move(ptr)) {}

    void Configure(pybind11::kwargs const& kwargs);

public:
    void SetOption(std::string const& option_name, pybind11::object const& option_value);

    [[nodiscard]] std::unordered_set<std::string_view> GetNeededOptions() const;

    // For pandas dataframes
    void Fit(pybind11::object dataframe, std::string name, pybind11::kwargs const& kwargs);

    void Fit(std::string const& path, char separator, bool has_header,
             pybind11::kwargs const& kwargs);

    pybind11::int_ Execute(pybind11::kwargs const& kwargs);
};

template <typename AlgorithmType, typename Base>
class PyAlgorithm : public Base {
    static_assert(std::is_base_of_v<PyAlgorithmBase, Base>);

public:
    PyAlgorithm() : Base(std::make_unique<AlgorithmType>()) {}
};

}  // namespace python_bindings
