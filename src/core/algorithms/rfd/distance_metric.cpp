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

bool IsPlaceholder(model::TypeId id) {
    return id == model::TypeId::kNull || id == model::TypeId::kEmpty ||
           id == model::TypeId::kUndefined;
}

std::optional<model::TypeId> ComparableTypeId(model::Type const* type, std::byte const* first,
                                              std::byte const* second) {
    if (type->GetTypeId() != model::TypeId::kMixed) return type->GetTypeId();
    model::TypeId const left_id = model::MixedType::RetrieveTypeId(first);
    model::TypeId const right_id = model::MixedType::RetrieveTypeId(second);
    if (IsPlaceholder(left_id) || IsPlaceholder(right_id) || left_id != right_id) {
        return std::nullopt;
    }
    return left_id;
}

class EqualityMetricImpl : public util::ICustomMetric {
public:
    double Dist(model::Type const* type, std::byte const* first,
                std::byte const* second) const override {
        if (ComparableTypeId(type, first, second) == std::nullopt) return kDissimilarDist;
        bool const equal = type->GetTypeId() == model::TypeId::kMixed
                                   ? static_cast<model::MixedType const*>(type)->Compare(
                                             first, second) == model::CompareResult::kEqual
                                   : type->Compare(first, second) == model::CompareResult::kEqual;
        return equal ? 0.0 : kDissimilarDist;
    }
};

class LevenshteinMetricImpl : public util::ICustomMetric {
public:
    double Dist(model::Type const* type, std::byte const* first,
                std::byte const* second) const override {
        std::string left_str;
        std::string right_str;
        if (type->GetTypeId() == model::TypeId::kMixed) {
            if (ComparableTypeId(type, first, second) == std::nullopt) return kDissimilarDist;
            auto const* mixed = static_cast<model::MixedType const*>(type);
            left_str = mixed->ValueToString(first);
            right_str = mixed->ValueToString(second);
        } else {
            left_str = type->ValueToString(first);
            right_str = type->ValueToString(second);
        }
        if (left_str.empty() && right_str.empty()) return 0.0;
        std::size_t const max_len = std::max(left_str.size(), right_str.size());
        if (max_len == 0) return 0.0;
        return static_cast<double>(util::LevenshteinDistance(left_str, right_str)) /
               static_cast<double>(max_len);
    }
};

class AbsoluteDifferenceMetricImpl : public util::ICustomMetric {
public:
    double Dist(model::Type const* type, std::byte const* first,
                std::byte const* second) const override {
        if (type->GetTypeId() == model::TypeId::kMixed) {
            std::optional<model::TypeId> const common_id = ComparableTypeId(type, first, second);
            if (common_id == std::nullopt) return kDissimilarDist;
            auto const* mixed = static_cast<model::MixedType const*>(type);
            model::TypeId const left_id = *common_id;
            if (left_id != model::TypeId::kInt && left_id != model::TypeId::kDouble) {
                throw config::ConfigurationError(
                        "abs_diff metric requires a numeric column, got " +
                        model::CreateType(left_id, mixed->IsNullEqNull())->ToString());
            }
            auto value_type = model::CreateType(left_id, mixed->IsNullEqNull());
            return Dist(value_type.get(), model::MixedType::RetrieveValue(first),
                        model::MixedType::RetrieveValue(second));
        }
        if (type->GetTypeId() != model::TypeId::kInt &&
            type->GetTypeId() != model::TypeId::kDouble) {
            throw config::ConfigurationError("abs_diff metric requires a numeric column, got " +
                                             type->ToString());
        }
        auto const* numeric = static_cast<model::INumericType const*>(type);
        double const left_value = numeric->GetValueAs<double>(first);
        double const right_value = numeric->GetValueAs<double>(second);
        double const absolute_difference = std::abs(left_value - right_value);
        double const max_absolute = std::max(std::abs(left_value), std::abs(right_value));
        if (max_absolute == 0.0) return 0.0;
        return absolute_difference / max_absolute;
    }
};

class AbsoluteThresholdMetricImpl : public util::ICustomMetric {
public:
    explicit AbsoluteThresholdMetricImpl(double tolerance) : tolerance_(tolerance) {}

    double Dist(model::Type const* type, std::byte const* first,
                std::byte const* second) const override;

private:
    double tolerance_;
};

}  // namespace

double AbsoluteThresholdMetricImpl::Dist(model::Type const* type, std::byte const* first,
                                         std::byte const* second) const {
    if (type->GetTypeId() == model::TypeId::kMixed) {
        std::optional<model::TypeId> const common_id = ComparableTypeId(type, first, second);
        if (common_id == std::nullopt) return kDissimilarDist;
        auto const* mixed = static_cast<model::MixedType const*>(type);
        model::TypeId const left_id = *common_id;
        if (left_id != model::TypeId::kInt && left_id != model::TypeId::kDouble) {
            throw config::ConfigurationError(
                    "abs_threshold metric requires a numeric column, got " +
                    model::CreateType(left_id, mixed->IsNullEqNull())->ToString());
        }
        auto value_type = model::CreateType(left_id, mixed->IsNullEqNull());
        return Dist(value_type.get(), model::MixedType::RetrieveValue(first),
                    model::MixedType::RetrieveValue(second));
    }
    if (type->GetTypeId() != model::TypeId::kInt && type->GetTypeId() != model::TypeId::kDouble) {
        throw config::ConfigurationError("abs_threshold metric requires a numeric column, got " +
                                         type->ToString());
    }
    auto const* numeric = static_cast<model::INumericType const*>(type);
    double const difference =
            std::abs(numeric->GetValueAs<double>(first) - numeric->GetValueAs<double>(second));
    return (difference <= tolerance_) ? 0.0 : kDissimilarDist;
}

std::shared_ptr<util::ICustomMetric> EqualityMetric() {
    return std::make_shared<EqualityMetricImpl>();
}

std::shared_ptr<util::ICustomMetric> LevenshteinMetric() {
    return std::make_shared<LevenshteinMetricImpl>();
}

std::shared_ptr<util::ICustomMetric> AbsoluteDifferenceMetric() {
    return std::make_shared<AbsoluteDifferenceMetricImpl>();
}

std::shared_ptr<util::ICustomMetric> AbsoluteThresholdMetricFactory(double tolerance) {
    return std::make_shared<AbsoluteThresholdMetricImpl>(tolerance);
}

}  // namespace algos::rfd
