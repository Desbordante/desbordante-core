#pragma once

#include <cassert>
#include <cstddef>
#include <format>
#include <variant>
#include <vector>

#include "core/config/custom_metric/custom_metrics/type.h"
#include "core/config/indices/type.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"

namespace util {
/// @brief CustomMetricsHolder is aimed to save a lot of `dynamic_cast`s for `IMetrizableType`-based
/// metrics
/// You should always prefer CustomMetricsHolder over plain
/// `std::vector<std::shared_ptr<ICustomMetric>>` until you have custom type handling
class CustomMetricsHolder {
private:
    std::vector<std::variant<model::Type const*, model::IMetrizableType const*>> types_;
    config::CustomMetricsType metrics_;

public:
    // This method is intended to be used only in Option constructor
    config::CustomMetricsType* ValuePtr() {
        return &metrics_;
    }

    void SetTypes(model::TypedRelationData const& typed_relation_data,
                  config::IndicesType const& column_indices) {
        assert(metrics_.size() == column_indices.size());
        assert(((void)"SetTypes cannot be called twice", types_.empty()));

        types_.reserve(metrics_.size());
        for (std::size_t i = 0; i < column_indices.size(); ++i) {
            auto const& column_data = typed_relation_data.GetColumnData(column_indices[i]);
            auto const* type = &column_data.GetType();
            auto const* metrizable_type = dynamic_cast<model::IMetrizableType const*>(type);
            if (metrizable_type) {
                types_.push_back(metrizable_type);
            } else {
                if (metrics_[i]->RequiresMetrizableType()) {
                    throw config::ConfigurationError(std::format(
                            "Metric for column {} requires a metrizable type, but type {} is not "
                            "metrizable",
                            column_data.GetColumn()->GetName(), type->ToString()));
                }
                types_.push_back(type);
            }
        }
    }

    double Dist(std::byte const* a, std::byte const* b, std::size_t idx) const {
        auto has_value = [](auto const* type) { return type != nullptr; };
        assert(((void)"Custom metric holder must be initialized first",
                std::visit(has_value, types_[idx])));

        return std::visit(
                [a, b, idx, this](auto const* type) { return metrics_[idx]->Dist(type, a, b); },
                types_[idx]);
    }
};
}  // namespace util
