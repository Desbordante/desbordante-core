#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "imetrizable_type.h"
#include "type.h"

namespace pac::model {
using Tuple = std::vector<std::byte const*>;
using Metric = std::function<double(Tuple const&, Tuple const&)>;
using Comparer = std::function<int(Tuple const&, Tuple const&)>;

/// @brief Provides operations to compare and calculate distance between fixed-size tuples of values
class MetrizableTupleType {
private:
    std::vector<::model::Type const*> types_;
    std::vector<::model::IMetrizableType const*> metrizable_types_;
    Metric metric_;
    Comparer comparer_;

    // Manhattan metric and "rectangle comparer" on a cartesian product of metric spaces (defined by
    // IMetrizableType) form a space, in which intervals are parallelepipeds.
    // However, their main usage is a tuple with a single element.
    double ManhattanMetric(Tuple const& x, Tuple const& y) const;
    int RectangleComparer(Tuple const& x, Tuple const& y) const;

public:
    MetrizableTupleType(std::vector<::model::Type const*>&& types, Metric&& metric = Metric{},
                        Comparer&& comparer = Comparer{});

    double Dist(Tuple const& x, Tuple const& y) const {
        assert(x.size() >= types_.size());
        assert(y.size() >= types_.size());

        return metric_(x, y);
    }

    int Compare(Tuple const& x, Tuple const& y) const {
        assert(x.size() >= types_.size());
        assert(y.size() >= types_.size());

        return comparer_(x, y);
    }

    std::string ValueToString(Tuple const& value) const;

    std::vector<::model::Type const*> const& GetTypes() const {
        return types_;
    }
};
}  // namespace pac::model
