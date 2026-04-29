#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <sstream>

#include "core/config/exceptions.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"

namespace config {
/// @brief User-defined metric on a single typed column
/// Together with @c PyCustomMetric, @c DynamicCustomMetric and @c StaticCustomMetric provides
/// a convenient user interface (especially, in Python) without extra overhead on conversions
// NOTE: these objects are wrapped in `shared_ptr`, and this encourages user to use the same object
// for several columns. Keep it in mind if you are planning to implement some complex internal state
class ICustomMetric {
public:
    virtual ~ICustomMetric() = default;

    virtual double Dist(model::Type const* type, std::byte const* first,
                        std::byte const* second) const = 0;
};

/// @brief Provides a convenient way to define custom metric, when column type is known in advance
/// WARN: Ignores real column type. Passing incorrect type leads to undefined behaviour
template <typename ArgType>
class StaticCustomMetric : public ICustomMetric {
private:
    using Metric = std::function<double(ArgType const&, ArgType const&)>;

    Metric metric_;

public:
    explicit StaticCustomMetric(Metric metric) : metric_(std::move(metric)) {}

    double Dist(model::Type const*, std::byte const* first,
                std::byte const* second) const override {
        return metric_(model::Type::GetValue<ArgType>(first),
                       model::Type::GetValue<ArgType>(second));
    }
};

/// @brief A custom metric, which uses real column type
class DynamicCustomMetric : public ICustomMetric {
private:
    using Metric = std::function<double(model::Type const*, std::byte const*, std::byte const*)>;

    Metric metric_;

public:
    explicit DynamicCustomMetric(Metric metric) : metric_(std::move(metric)) {}

    double Dist(model::Type const* type, std::byte const* first,
                std::byte const* second) const override {
        return metric_(type, first, second);
    }
};

/// @brief A default value for custom metric option
/// Uses default metric for the type. Works only with metrizable types
class DefaultCustomMetric : public ICustomMetric {
private:
    static model::IMetrizableType const* ConvertType(model::Type const* type) {
        auto const* metr_type = dynamic_cast<model::IMetrizableType const*>(type);
        if (!metr_type) {
            std::ostringstream msg;
            msg << "Cannot use default metric, because column type " << type->ToString()
                << " is not metrizable. Consider defining custom metric";
            throw ConfigurationError(msg.str());
        }
        return metr_type;
    }

public:
    double Dist(model::Type const* type, std::byte const* first,
                std::byte const* second) const override {
        return ConvertType(type)->Dist(first, second);
    }
};
}  // namespace config
