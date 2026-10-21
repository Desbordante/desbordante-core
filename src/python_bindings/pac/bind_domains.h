#pragma once

#include <pybind11/pybind11.h>

#include <string>
#include <utility>

#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/model/tuple.h"
#include "python_bindings/py_util/value_to_py.h"

namespace python_bindings {
class PyDomain final : public pac::model::IDomain {
private:
    pybind11::object dist_from_domain_;
    std::string name_;

public:
    PyDomain(pybind11::object dist_from_domain, std::string&& name)
        : dist_from_domain_(std::move(dist_from_domain)), name_(std::move(name)) {}

    double DistFromDomain(pac::model::Tuple const& value) const override {
        return pybind11::cast<double>(
                dist_from_domain_(ValuesToPy(tuple_type_->GetTypes(), value)));
    }

    std::string ToString() const override {
        return name_;
    }
};

void BindDomains(pybind11::module_& pac_module);
}  // namespace python_bindings
