#include "core/algorithms/rfd/distance_metric.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>

#include "core/config/exceptions.h"
#include "core/model/types/create_type.h"
#include "core/model/types/mixed_type.h"
#include "core/model/types/numeric_type.h"
#include "core/model/types/type.h"
#include "core/util/levenshtein_distance.h"

namespace algos::rfd {

namespace {

constexpr double kDissimilarDist = 1.0;

std::optional<model::TypeId> ComparableTypeId(model::Type const* type, std::byte const* first,
                                              std::byte const* second) {
    if (type->GetTypeId() != model::TypeId::kMixed) return type->GetTypeId();
    model::TypeId const left_id = model::MixedType::RetrieveTypeId(first);
    model::TypeId const right_id = model::MixedType::RetrieveTypeId(second);

    if (left_id != right_id) return std::nullopt;

    static model::TypeId const kInvalidTypes[] = {model::TypeId::kNull, model::TypeId::kEmpty,
                                                  model::TypeId::kUndefined};

    if (std::ranges::find(kInvalidTypes, left_id) != std::end(kInvalidTypes)) return std::nullopt;
    return left_id;
}

std::optional<std::pair<double, double>> NumericValues(model::Type const* type,
                                                       std::byte const* first,
                                                       std::byte const* second,
                                                       std::string_view metric) {
    std::optional<model::TypeId> const id = ComparableTypeId(type, first, second);
    if (!id) return std::nullopt;
    if (*id != model::TypeId::kInt && *id != model::TypeId::kDouble) {
        throw std::domain_error(std::string(metric) + " metric requires a numeric column");
    }
    if (type->GetTypeId() == model::TypeId::kMixed) {
        first = model::MixedType::RetrieveValue(first);
        second = model::MixedType::RetrieveValue(second);
    }
    auto as_double = [id](std::byte const* v) {
        return *id == model::TypeId::kInt ? double(model::Type::GetValue<model::Int>(v))
                                          : model::Type::GetValue<model::Double>(v);
    };
    return std::pair{as_double(first), as_double(second)};
}

}  // namespace

std::shared_ptr<util::ICustomMetric> EqualityMetric() {
    return std::make_shared<util::DynamicCustomMetric>(
            [](model::Type const* type, std::byte const* first, std::byte const* second) {
                if (!ComparableTypeId(type, first, second)) return kDissimilarDist;
                return type->Compare(first, second) == model::CompareResult::kEqual
                               ? 0.0
                               : kDissimilarDist;
            });
}

std::shared_ptr<util::ICustomMetric> LevenshteinMetric() {
    return std::make_shared<util::DynamicCustomMetric>(
            [](model::Type const* type, std::byte const* first, std::byte const* second) {
                if (!ComparableTypeId(type, first, second)) return kDissimilarDist;
                std::string const l = type->ValueToString(first);
                std::string const r = type->ValueToString(second);
                std::size_t const max_len = std::max(l.size(), r.size());
                if (max_len == 0) return 0.0;
                return static_cast<double>(util::LevenshteinDistance(l, r)) /
                       static_cast<double>(max_len);
            });
}

std::shared_ptr<util::ICustomMetric> AbsoluteDifferenceMetric() {
    return std::make_shared<util::DynamicCustomMetric>(
            [](model::Type const* type, std::byte const* first, std::byte const* second) {
                auto values = NumericValues(type, first, second, "abs_diff");
                if (!values) return kDissimilarDist;

                double const absolute_difference = std::abs(values->first - values->second);
                double const max_absolute =
                        std::max(std::abs(values->first), std::abs(values->second));
                if (max_absolute == 0.0) return 0.0;
                return absolute_difference / max_absolute;
            });
}

std::shared_ptr<util::ICustomMetric> AbsoluteThresholdMetricFactory(double tolerance) {
    return std::make_shared<util::DynamicCustomMetric>(
            [tolerance](model::Type const* type, std::byte const* first, std::byte const* second) {
                auto values = NumericValues(type, first, second, "abs_threshold");
                if (!values) return kDissimilarDist;

                double const absolute_difference = std::abs(values->first - values->second);
                return (absolute_difference <= tolerance) ? 0.0 : kDissimilarDist;
            });
}

}  // namespace algos::rfd
