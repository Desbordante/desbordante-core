# pragma once

#include <optional>

#include "data/DataFrame.h"
#include "predicates/SingleAttributePredicate.h"
#include "attribute_set.h"

namespace algos::fastod {

class CanonicalOD {
private:
    AttributeSet const context_;
    std::optional<SingleAttributePredicate> const left_;
    int const right_;
    static int split_check_count_;
    static int swap_check_count_;

public:
    CanonicalOD(const AttributeSet& context, const SingleAttributePredicate& left, int right) noexcept;
    CanonicalOD(const AttributeSet& context, int right) noexcept;

    bool IsValid(const DataFrame& data, double error_rate_threshold) const noexcept;
    std::string ToString() const noexcept;

    friend bool operator==(CanonicalOD const& x, CanonicalOD const& y);
    friend bool operator<(CanonicalOD const& x, CanonicalOD const& y);
};

} // namespace algos::fatod
