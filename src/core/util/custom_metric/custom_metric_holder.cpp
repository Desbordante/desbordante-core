#include "core/util/custom_metric/custom_metric_holder.h"

#include <cassert>
#include <cstddef>
#include <format>
#include <memory>
#include <variant>

#include "core/config/exceptions.h"
#include "core/model/types/imetrizable_type.h"
#include "core/util/custom_metric/custom_metric.h"

using namespace util;

namespace {
template <typename... T>
class Overloaded : T... {
    using T::operator()...;
};

using TypeVariant = std::variant<model::Type const*, model::IMetrizableType const*>;
}  // namespace

void CustomMetricHolder::SetType(model::Type const* type) {
    std::visit(Overloaded{[&type](std::shared_ptr<ICustomMetric> const&) -> TypeVariant {
                              return type;
                          },
                          [&type](std::shared_ptr<IMetrizableCustomMetric> const&) -> TypeVariant {
                              auto metr_type = dynamic_cast<model::IMetrizableType const*>(type);
                              if (metr_type) {
                                  return metr_type;
                              }
                              throw config::ConfigurationError(
                                      std::format("Provided metric requires a metrizable type, but "
                                                  "type {} is not metrizable",
                                                  type->ToString()));
                          }},
               metric_);
}

double CustomMetricHolder::Dist(std::byte const* a, std::byte const* b) const {
    assert(((void)"Custom metric holder must be initialized first",
            std::visit([](auto const* p) { return p == nullptr; }, type_)));
    assert(((void)"Metric cannot be nullptr, do you use CustomMetricOption?",
            std::visit([](auto const& p) { return p == nullptr; }, metric_)));

    return std::visit(
            [a, b, this](auto const& metric) {
                return std::visit([a, b, &metric](auto const* type) { return metric(type); }, type_)
            },
            metric_);
}
