#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <vector>

#include "core/config/exceptions.h"
#include "core/model/types/imetrizable_type.h"
#include "core/model/types/type.h"

namespace config {
/// @brief User-defined metric on a set of columns, that treats values as tuples (vectors)
/// Together with @c PyCustomVectorMetric and @c CustomVectorMetric provides
/// a convenient user interface (especially, in Python) without extra overhead on conversions
// NOTE: these objects are wrapped in `shared_ptr`, and this encourages user to use the same object
// for several columns. Keep it in mind if you are planning to implement some complex internal state
/// WARN: NULL value is represented by nullptr
class ICustomVectorMetric {
protected:
    using Types = std::vector<model::Type const*>;
    using Values = std::vector<std::byte const*>;

public:
    virtual ~ICustomVectorMetric() = default;

    virtual double Dist(Types const& types, Values const& first, Values const& second) const = 0;
};

/// @brief A custom vector metric, which uses real column types
class CustomVectorMetric : public ICustomVectorMetric {
private:
    using Metric = std::function<double(Types const&, Values const&, Values const&)>;

    Metric metric_;

public:
    explicit CustomVectorMetric(Metric metric) : metric_(std::move(metric)) {}

    double Dist(Types const& types, Values const& first, Values const& second) const override {
        return metric_(types, first, second);
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
            throw ConfigurationError(msg.str());
        }
        return metr_type;
    }

public:
    double Dist(Types const& types, Values const& first, Values const& second) const override {
        assert(types.size() == first.size() && types.size() == second.size());

        double result = 0;
        for (std::size_t i = 0; i < types.size(); ++i) {
            result += std::pow(ConvertType(types[i])->Dist(first[i], second[i]), 2);
        }
        return std::sqrt(result);
    }
};
}  // namespace config
