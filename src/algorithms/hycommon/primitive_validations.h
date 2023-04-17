#pragma once
#include <vector>

#include "types.h"

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

    std::vector<RawPrimitive> const& invalid_instances() const noexcept {
        return invalid_instances_;
    }
    std::vector<RawPrimitive>& invalid_instances() noexcept {
        return invalid_instances_;
    }
    IdPairs const& comparison_suggestions() const noexcept {
        return comparison_suggestions_;
    }
    IdPairs& comparison_suggestions() noexcept {
        return comparison_suggestions_;
    }
    unsigned count_validations() const noexcept {
        return count_validations_;
    }
    void set_count_validations(unsigned count_validations) noexcept {
        count_validations_ = count_validations;
    }
    unsigned count_intersections() const noexcept {
        return count_intersections_;
    }
    void set_count_intersections(unsigned count_intersections) noexcept {
        count_intersections_ = count_intersections;
    }
};

}  // namespace algos::hy
