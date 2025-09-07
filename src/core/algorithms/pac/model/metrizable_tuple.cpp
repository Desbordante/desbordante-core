#include "algorithms/pac/model/metrizable_tuple.h"

#include <cstddef>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include "builtin.h"
#include "exceptions.h"

namespace pac::model {
double MetrizableTupleType::ManhattanMetric(Tuple const& x, Tuple const& y) const {
    double metric = 0;
    for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
        metric += metrizable_types_[i]->Dist(x[i], y[i]);
    }
    return metric;
}

int MetrizableTupleType::RectangleComparer(Tuple const& x, Tuple const& y) const {
    for (std::size_t i = 0; i < types_.size(); ++i) {
        auto comp_res = types_[i]->Compare(x[i], y[i]);
        if (comp_res != ::model::CompareResult::kNotEqual) {
            return static_cast<int>(comp_res);
        }
    }
    return 0;
};

MetrizableTupleType::MetrizableTupleType(std::vector<::model::Type const*>&& types, Metric&& metric,
                                         Comparer&& comparer)
    : types_(std::move(types)), metric_(std::move(metric)), comparer_(std::move(comparer)) {
    using namespace std::placeholders;

    if (!metric_) {
        metrizable_types_ = {};
        for (auto const* type : types_) {
            auto const* metrizable_type = dynamic_cast<::model::IMetrizableType const*>(type);
            if (!metrizable_type) {
                throw config::ConfigurationError(
                        "All attribute types must be metrizable when default metric is used");
            }
            metrizable_types_.push_back(metrizable_type);
        }
        metric_ = std::bind(&MetrizableTupleType::ManhattanMetric, this, _1, _2);
    }
    if (!comparer_) {
        comparer_ = std::bind(&MetrizableTupleType::RectangleComparer, this, _1, _2);
    }
}

std::string MetrizableTupleType::ValueToString(Tuple const& value) const {
    assert(value.size() >= types_.size());

    if (types_.size() == 1) {
        return types_.front()->ValueToString(value.front());
    }
    std::ostringstream oss;
    oss << '{';
    for (std::size_t i = 0; i < types_.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << types_[i]->ValueToString(value[i]);
    }
    oss << '}';
    return oss.str();
}
}  // namespace pac::model
