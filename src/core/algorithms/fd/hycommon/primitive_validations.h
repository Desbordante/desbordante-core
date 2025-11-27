#pragma once
#include <vector>

#include "core/algorithms/fd/hycommon/types.h"

namespace algos::hy {

template <typename RawPrimitive>
class PrimitiveValidations {
private:
    std::vector<RawPrimitive> invalid_instances_;
    IdPairs comparison_suggestions_;

    unsigned count_validations_ = 0;
    unsigned count_intersections_ = 0;

public:
    void Add(PrimitiveValidations const& other) {
        invalid_instances_.insert(invalid_instances_.end(), other.invalid_instances_.begin(),
                                  other.invalid_instances_.end());

        comparison_suggestions_.insert(comparison_suggestions_.end(),
                                       other.comparison_suggestions_.begin(),
                                       other.comparison_suggestions_.end());

        count_intersections_ += other.count_intersections_;
        count_validations_ += other.count_validations_;
    }

    std::vector<RawPrimitive> const& InvalidInstances() const noexcept {
        return invalid_instances_;
    }

    std::vector<RawPrimitive>& InvalidInstances() noexcept {
        return invalid_instances_;
    }

    IdPairs const& ComparisonSuggestions() const noexcept {
        return comparison_suggestions_;
    }

    IdPairs& ComparisonSuggestions() noexcept {
        return comparison_suggestions_;
    }

    unsigned CountValidations() const noexcept {
        return count_validations_;
    }

    void SetCountValidations(unsigned count_validations) noexcept {
        count_validations_ = count_validations;
    }

    unsigned CountIntersections() const noexcept {
        return count_intersections_;
    }

    void SetCountIntersections(unsigned count_intersections) noexcept {
        count_intersections_ = count_intersections;
    }
};

}  // namespace algos::hy
