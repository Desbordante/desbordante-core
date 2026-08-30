#pragma once

#include <cstddef>
#include <variant>

#include "core/config/custom_metric/custom_metric/type.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"

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

    void SetType(model::Type const* type);
    double Dist(std::byte const* a, std::byte const* b) const;
};
}  // namespace util
