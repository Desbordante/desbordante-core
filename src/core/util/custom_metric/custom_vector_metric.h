#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/config/exceptions.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"
#include "core/util/export.h"

// TODO: vector metrics

namespace util {
/// @brief User-defined metric on a set of columns, that treats values as tuples (vectors)
/// Together with @c PyCustomVectorMetric and @c CustomVectorMetric provides
/// a convenient user interface (especially, in Python) without extra overhead on conversions
// NOTE: these objects are wrapped in `shared_ptr`, and this encourages user to use the same object
// for several columns. Keep it in mind if you are planning to implement some complex internal state
/// WARN: NULL value is represented by nullptr
class DESBORDANTE_EXPORT ICustomVectorMetric {
protected:
    using Types = std::vector<model::Type const*>;
    using Values = std::vector<std::byte const*>;

public:
    /// @brief A user-defined metric populated with type
    /// Even though interface doesn't bind TypedMetric to a column, it is highly recommended to use
    /// a dedicated TypedMetric for each column to avoid unexpected results (because this type is
    /// generally user-defined)
    class DESBORDANTE_EXPORT ITypedMetric {
    public:
        virtual ~ITypedMetric() = default;
        virtual double operator()(Values const& a, Values const& b) const = 0;
    };

    virtual ~ICustomVectorMetric() = default;

    // NOTE: column_name should be used only to emit more informative error messages.
    // Do not try to reconstruct any column info depending on it
    virtual std::unique_ptr<ITypedMetric> SetTypes(
            Types const& types, std::vector<std::string> const& column_names = {}) const = 0;
};

/// @brief A custom vector metric, which uses real column types
class CustomVectorMetric : public ICustomVectorMetric {
private:
    using Metric = std::function<double(Types const&, Values const&, Values const&)>;

    Metric metric_;

public:
    class TypedMetric : public ITypedMetric {
    private:
        Metric metric_;
        Types types_;

    public:
        TypedMetric(Metric metric, Types types)
            : metric_(std::move(metric)), types_(std::move(types)) {}

        double operator()(Values const& a, Values const& b) const override {
            return metric_(types_, a, b);
        }
    };

    explicit CustomVectorMetric(Metric metric) : metric_(std::move(metric)) {}

    std::unique_ptr<ITypedMetric> SetTypes(Types const& types,
                                           std::vector<std::string> const&) const {
        return std::make_unique<TypedMetric>(metric_, types);
    }
};

/// @brief A default value for custom vector metric option
/// Uses euclidean metric and default metrics for individual coordinates.
/// Works only with metrizable types
class DefaultCustomVectorMetric : public ICustomVectorMetric {
private:
    static model::IMetrizableType const* ConvertType(model::Type const* type) {
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
    class TypedMetric : public ITypedMetric {
    private:
        std::vector<model::IMetrizableType const*> types_;

    public:
        TypedMetric(Types const& types, std::vector<std::string> const& column_names) {
            assert(column_names.empty() || types.size() == column_names.size());

            types_.reserve(types.size());
            for (std::size_t i = 0; i < types.size(); ++i) {
                auto metr_type = dynamic_cast<model::IMetrizableType const*>(types[i]);
                if (!metr_type) {
                    std::ostringstream msg;
                    msg << "Cannot use default metric, because column type "
                        << types[i]->ToString();
                    if (!column_names.empty()) {
                        msg << " for column " << column_names[i];
                    }
                    msg << " is not metrizable. Consider using custom metric";
                    throw config::ConfigurationError(msg.str());
                }
                types_.push_back(metr_type);
            }
        }

        double operator()(Values const& a, Values const& b) const override {
            assert(types_.size() == a.size() && types_.size() == b.size());

            double result = 0;
            for (std::size_t i = 0; i < types_.size(); ++i) {
                if (a[i] && b[i]) {
                    double dist = ConvertType(types_[i])->Dist(a[i], b[i]);
                    result += dist * dist;
                }
            }
            return std::sqrt(result);
        }
    };

    std::unique_ptr<ITypedMetric> SetTypes(
            Types const& types, std::vector<std::string> const& column_names = {}) const override {
        return std::make_unique<TypedMetric>(types, column_names);
    }
};
}  // namespace util
