#include "python_bindings/py_util/value_to_py.h"

#include <pybind11/pybind11.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <pybind11/pytypes.h>

#include "core/model/types/builtin.h"
#include "core/model/types/create_type.h"
#include "core/model/types/mixed_type.h"
#include "core/model/types/type.h"

using namespace model;
namespace py = pybind11;

namespace {
py::object DateToPyObject(Date const& date) {
    auto py_date = py::module_::import("datetime").attr("date");

    auto ymd = date.year_month_day();
    // Unlike the Python API, pybind cannot cast types here
    return py_date(static_cast<unsigned>(ymd.year), static_cast<unsigned>(ymd.month),
                   static_cast<unsigned>(ymd.day));
}

py::object MixedToPy(model::Type const* type, std::byte const* value) {
    auto const* mixed_type = dynamic_cast<model::MixedType const*>(type);
    assert(mixed_type);
    auto real_type = CreateType(mixed_type->RetrieveTypeId(value), mixed_type->IsNullEqNull());
    return python_bindings::ValueToPy(real_type.get(), mixed_type->RetrieveValue(value));
}
}  // namespace

namespace python_bindings {
py::object ValueToPy(Type const* type, std::byte const* value) {
    if (!value) {
        return py::none{};
    }

    switch (type->GetTypeId()) {
        case TypeId::kBool:
            return py::bool_(Type::GetValue<bool>(value));
        case TypeId::kInt:
            return py::int_(Type::GetValue<Int>(value));
        case TypeId::kDouble:
            return py::float_(Type::GetValue<Double>(value));
        case TypeId::kBigInt:
            // Parameters:
            // 1. C string
            // 2. First character after integer representation (nullptr -- take the entire string)
            // 3. Base (0 -- auto-detect)
            return py::reinterpret_steal<py::object>(PyLong_FromString(
                    static_cast<std::string const&>(Type::GetValue<BigInt>(value)).c_str(), nullptr,
                    0));
        case TypeId::kString:
            return py::str(Type::GetValue<String>(value));
        case TypeId::kDate:
            return DateToPyObject(Type::GetValue<Date>(value));
        case TypeId::kNull:
        case TypeId::kEmpty:
        case TypeId::kUndefined:
            return py::none{};
        case TypeId::kMixed:
            return MixedToPy(type, value);
    }
    throw std::runtime_error("Unknown type: " + type->ToString());
}

std::vector<pybind11::object> ValuesToPy(std::vector<model::Type const*> const& types,
                                         std::vector<std::byte const*> const& values) {
    assert(types.size() == values.size());

    std::vector<pybind11::object> py_objects;
    py_objects.reserve(types.size());
    std::ranges::transform(types, values, std::back_inserter(py_objects), &ValueToPy);
    return py_objects;
}
}  // namespace python_bindings
