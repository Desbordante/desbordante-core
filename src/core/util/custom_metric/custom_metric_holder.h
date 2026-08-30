#pragma once

#include <cassert>
#include <cstddef>
#include <format>
#include <variant>

#include "core/config/custom_metric/custom_metric/type.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"
#include "core/util/custom_metric/custom_metric.h"

namespace util {
/// @brief CustomMetricHolder is aimed to save a lot of `dynamic_cast`s for `IMetrizableType`-based
/// metrics
/// You should always prefer CustomMetricHolder over plain `std::shared_ptr<ICustomMetric>` until
/// you have custom type handling
class CustomMetricHolder {
private:
    std::variant<model::Type const*, model::IMetrizableType const*> type_;
    config::CustomMetricType metric_;

public:
    // This method is intended to be used only in Option constructor
    config::CustomMetricType* ValuePtr() {
        return &metric_;
    }

    void SetType(model::Type const* type) {
        auto metr_type = dynamic_cast<model::IMetrizableType const*>(type);
        if (metr_type) {
            type_ = metr_type;
        } else {
            if (metric_->RequiresMetrizableType()) {
                auto type_name = type->ToString();
                throw config::ConfigurationError(std::format(
                        "Provided metric requires a metrizable type, but type {} is not metrizable",
                        type->ToString()));
            }
            type_ = type;
        }
    }

    double Dist(std::byte const* a, std::byte const* b) const {
        auto valueless = [](auto const* type) { return type == nullptr; };
        assert(((void)"Custom metric holder must be initialized first",
                std::visit(valueless, type_)));

        return std::visit([a, b, this](auto const* type) { return metric_->Dist(type, a, b); },
                          type_);
    }
};
}  // namespace util
