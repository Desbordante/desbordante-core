#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
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
public:
    /// @brief A user-defined metric populated with type
    /// Even though interface doesn't bind TypedMetric to a column, it is highly recommended to use
    /// a dedicated TypedMetric for each column to avoid unexpected results (because this type is
    /// generally user-defined)
    class DESBORDANTE_EXPORT ITypedMetric {
    public:
        virtual ~ITypedMetric() = default;
        virtual double operator()(std::byte const* a, std::byte const* b) const = 0;
    };

    virtual ~ICustomMetric() = default;

    // NOTE: column_name should be used only to emit more informative error messages.
    // Do not try to reconstruct any column info depending on it
    virtual std::unique_ptr<ITypedMetric> SetType(model::Type const* type,
                                                  std::string const& column_name = "") const = 0;
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
    class TypedMetric : public ITypedMetric {
    private:
        Metric metric_;

    public:
        explicit TypedMetric(Metric metric) : metric_(std::move(metric)) {}

        double operator()(std::byte const* a, std::byte const* b) const override {
            return metric_(GetValue(a), GetValue(b));
        }
    };

    explicit StaticCustomMetric(Metric metric) : metric_(std::move(metric)) {}

    std::unique_ptr<ITypedMetric> SetType(model::Type const*, std::string const&) const {
        return std::make_unique<TypedMetric>(metric_);
    }
};

/// @brief A custom metric, which uses real column type
class DynamicCustomMetric : public ICustomMetric {
private:
    using Metric = std::function<double(model::Type const*, std::byte const*, std::byte const*)>;

    Metric metric_;

public:
    class TypedMetric : public ITypedMetric {
    private:
        Metric metric_;
        model::Type const* type_;

    public:
        TypedMetric(Metric metric, model::Type const* type)
            : metric_(std::move(metric)), type_(type) {}

        double operator()(std::byte const* a, std::byte const* b) const override {
            return metric_(type_, a, b);
        }
    };

    explicit DynamicCustomMetric(Metric metric) : metric_(std::move(metric)) {}

    std::unique_ptr<ITypedMetric> SetType(model::Type const* type,
                                          std::string const&) const override {
        return std::make_unique<TypedMetric>(metric_, type);
    }
};

/// @brief A default value for custom metric option
/// Uses default metric for the type. Works only with metrizable types
class DefaultCustomMetric : public ICustomMetric {
public:
    class TypedMetric : public ITypedMetric {
    private:
        model::IMetrizableType const* type_;

    public:
        explicit TypedMetric(model::Type const* type, std::string const& column_name = "") {
            type_ = dynamic_cast<model::IMetrizableType const*>(type);
            if (!type_) {
                std::ostringstream msg;
                msg << "Cannot use default metric, because column " << "type " << type->ToString();
                if (!column_name.empty()) {
                    msg << " for column " << column_name;
                }
                msg << " is not metrizable. Consider defining custom metric";
                throw config::ConfigurationError(msg.str());
            }
        }

        double operator()(std::byte const* a, std::byte const* b) const override {
            if (!a || !b) {
                return 0;
            }
            return type_->Dist(a, b);
        }
    };

    std::unique_ptr<ITypedMetric> SetType(model::Type const* type,
                                          std::string const& column_name) const override {
        return std::make_unique<TypedMetric>(type, column_name);
    }
};
}  // namespace util
