#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <string>
#include <variant>

#include "core/config/exceptions.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"
#include "core/util/export.h"

namespace algos::pac_verifier {
using RawDataMetric = std::function<double(std::byte const*, std::byte const*, model::Type const*)>;
using StringDataMetric = std::function<double(std::string&&, std::string&&)>;
// Order matters: pybind11 tries types left-to-right
using ValueMetric = std::variant<std::nullptr_t, StringDataMetric, RawDataMetric>;

namespace detail {
// pybind11::type::of, which is used by our GetPyType, cannot work with standard types.
// So we need this wrapper around ValueMetric (which is std::variant).
// The only place this class is used is `get_opt_type` method of FDPACVerifier<true>.
class DESBORDANTE_EXPORT FakeValueMetric {
private:
    ValueMetric value_metric_;

public:
    FakeValueMetric(ValueMetric&& value_metric) : value_metric_(std::move(value_metric)) {}

    operator ValueMetric() {
        return value_metric_;
    }
};
}  // namespace detail

/// @brief Provides a convenient way to calculate distance between values in some column, either
/// using user-provided metric or a default one, and taking into account @c dist_from_null_is_infty
/// option
class ColumnMetric {
private:
    model::Type const* type_;
    // Either metr_type_ or raw_data_metric_ or string_data_metric_ is used.
    // It's invariant that exactly one of them is not nullptr.
    model::IMetrizableType const* metr_type_;
    RawDataMetric raw_data_metric_;
    StringDataMetric string_data_metric_;
    bool dist_from_null_is_infty_;

public:
    ColumnMetric() = default;
    ColumnMetric(ColumnMetric const&) = default;
    ColumnMetric& operator=(ColumnMetric const&) = default;
    ColumnMetric(ColumnMetric&&) = default;
    ColumnMetric& operator=(ColumnMetric&&) = default;
    ~ColumnMetric() = default;

    ColumnMetric(model::Type const* type, std::nullptr_t, bool dist_from_null_is_infty)
        : type_(type), dist_from_null_is_infty_(dist_from_null_is_infty) {
        metr_type_ = dynamic_cast<model::IMetrizableType const*>(type_);
        if (!metr_type_) {
            throw config::ConfigurationError("Cannot use default metric, because column type " +
                                             type_->ToString() + " is not metrizable");
        }
    }

    ColumnMetric(model::Type const* type, RawDataMetric&& raw_data_metric,
                 bool dist_from_null_is_infty)
        : type_(type),
          raw_data_metric_(std::move(raw_data_metric)),
          dist_from_null_is_infty_(dist_from_null_is_infty) {}

    ColumnMetric(model::Type const* type, StringDataMetric&& string_data_metric,
                 bool dist_from_null_is_infty)
        : type_(type),
          string_data_metric_(std::move(string_data_metric)),
          dist_from_null_is_infty_(dist_from_null_is_infty) {}

    double Dist(std::byte const* x, std::byte const* y) const {
        if (!x || !y) {
            return dist_from_null_is_infty_ ? std::numeric_limits<double>::infinity() : 0;
        }
        if (raw_data_metric_) {
            return raw_data_metric_(x, y, type_);
        }
        if (string_data_metric_) {
            return string_data_metric_(type_->ValueToString(x), type_->ValueToString(y));
        }
        assert(metr_type_);
        return metr_type_->Dist(x, y);
    }
};
}  // namespace algos::pac_verifier
