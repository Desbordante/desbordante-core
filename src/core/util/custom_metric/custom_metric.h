#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <optional>
#include <sstream>
#include <utility>

#include "core/config/exceptions.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"
#include "core/util/export.h"

namespace util {
/// @brief User-defined metric on a single typed column
/// Together with @c PyCustomMetric, @c DynamicCustomMetric and @c StaticCustomMetric provides
/// a convenient user interface (especially, in Python) without extra overhead on conversions
// NOTE: these objects are wrapped in `shared_ptr`, and this encourages user to use the same object
// for several columns. Keep it in mind if you are planning to implement some complex internal state
/// WARN: NULL value is represented by nullptr
class DESBORDANTE_EXPORT ICustomMetric {
protected:
    static model::IMetrizableType const* TryConvertType(model::Type const* type) {
        auto const* metr_type = dynamic_cast<model::IMetrizableType const*>(type);
        if (!metr_type) {
            std::ostringstream msg;
            msg << "Cannot use default metric, because column type " << type->ToString()
                << " is not metrizable. Consider defining custom metric";
            throw config::ConfigurationError(msg.str());
        }
        return metr_type;
    }

public:
    virtual ~ICustomMetric() = default;

    virtual double Dist(model::Type const* type, std::byte const* first,
                        std::byte const* second) const = 0;

    virtual double Dist(model::IMetrizableType const* type, std::byte const* first,
                        std::byte const* second) const {
        return Dist(static_cast<model::Type const*>(type), first, second);
    }

    virtual bool RequiresMetrizableType() const {
        return false;
    }
};

/// @brief Provides a convenient way to define custom metric, when column type is known in advance
/// WARN: Ignores real column type. Passing incorrect type leads to undefined behaviour
template <typename ArgType>
class StaticCustomMetric : public ICustomMetric {
private:
    using OptArg = std::optional<ArgType>;
    using Metric = std::function<double(OptArg const&, OptArg const&)>;

    Metric metric_;

    static OptArg GetValue(std::byte const* value) {
        if (!value) {
            return std::nullopt;
        }
        return model::Type::GetValue<ArgType>(value);
    }

public:
    explicit StaticCustomMetric(Metric metric) : metric_(std::move(metric)) {}

    double Dist(model::Type const*, std::byte const* first,
                std::byte const* second) const override {
        return metric_(GetValue(first), GetValue(second));
    }
};

/// @brief A custom metric, which uses real column type
class DynamicCustomMetric : public ICustomMetric {
private:
    using Metric = std::function<double(model::Type const*, std::byte const*, std::byte const*)>;
    using MetricForMetrizableType = std::function<double(model::IMetrizableType const*,
                                                         std::byte const*, std::byte const*)>;

    // Exactly one of them is not nullptr
    Metric metric_;
    MetricForMetrizableType metric_for_metrizable_type_;

public:
    explicit DynamicCustomMetric(Metric metric) : metric_(std::move(metric)) {}

    explicit DynamicCustomMetric(MetricForMetrizableType metric_for_metrizable_type)
        : metric_for_metrizable_type_(std::move(metric_for_metrizable_type)) {}

    double Dist(model::Type const* type, std::byte const* first,
                std::byte const* second) const override {
        assert((metric_ == nullptr) != (metric_for_metrizable_type_ == nullptr));

        if (metric_for_metrizable_type_) {
            return metric_for_metrizable_type_(TryConvertType(type), first, second);
        }
        return metric_(type, first, second);
    }

    double Dist(model::IMetrizableType const* type, std::byte const* first,
                std::byte const* second) const override {
        assert((metric_ == nullptr) != (metric_for_metrizable_type_ == nullptr));

        if (metric_for_metrizable_type_) {
            return metric_for_metrizable_type_(type, first, second);
        }
        return metric_(type, first, second);
    }

    bool RequiresMetrizableType() const override {
        return metric_for_metrizable_type_ != nullptr;
    }
};

/// @brief A default value for custom metric option
/// Uses default metric for the type. Works only with metrizable types
class DefaultCustomMetric : public ICustomMetric {
private:
public:
    // TODO(p-senichenkov): simply throw here? I. e. disallow direct calls (not through
    // CustomMetricHolder)
    double Dist(model::Type const* type, std::byte const* first,
                std::byte const* second) const override {
        if (!first || !second) {
            return 0;
        }
        return Dist(TryConvertType(type), first, second);
    }

    double Dist(model::IMetrizableType const* type, std::byte const* first,
                std::byte const* second) const override {
        if (!first || !second) {
            return 0;
        }
        return type->Dist(first, second);
    }

    bool RequiresMetrizableType() const override {
        return true;
    }
};
}  // namespace util
