#pragma once

#include <compare>
#include <list>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/tools/floating_point_comparison.hpp>

#include "model/types/builtin.h"
#include "model/types/double_type.h"

namespace model {

static double constexpr kRelativeTolerance =
        DoubleType::kDefaultEpsCount * std::numeric_limits<model::Double>::epsilon();

static bool IsEqual(double first_value, double second_value) {
    return boost::math::fpc::close_at_tolerance<model::Double>(kRelativeTolerance)(first_value,
                                                                                   second_value);
}

static bool Less(double first_value, double second_value) {
    return first_value < second_value && !IsEqual(first_value, second_value);
}

static bool Greater(double first_value, double second_value) {
    return first_value > second_value && !IsEqual(first_value, second_value);
}

static bool LessOrEqual(double first_value, double second_value) {
    return !Greater(first_value, second_value);
}

static bool GreaterOrEqual(double first_value, double second_value) {
    return !Less(first_value, second_value);
}

struct DFConstraint {
    double lower_bound;
    double upper_bound;

    auto operator<=>(DFConstraint const& other) const {
        if (!IsEqual(lower_bound, other.lower_bound)) {
            return Less(lower_bound, other.lower_bound) ? std::strong_ordering::less
                                                        : std::strong_ordering::greater;
        }
        if (!IsEqual(upper_bound, other.upper_bound)) {
            return Less(upper_bound, other.upper_bound) ? std::strong_ordering::less
                                                        : std::strong_ordering::greater;
        }
        return std::strong_ordering::equal;
    }

    bool operator==(DFConstraint const& other) const {
        return IsEqual(lower_bound, other.lower_bound) && IsEqual(upper_bound, other.upper_bound);
    }

    bool Contains(double value) const {
        return LessOrEqual(lower_bound, value) && LessOrEqual(value, upper_bound);
    }

    bool IsWithinExclusive(double value) const {
        return GreaterOrEqual(value, lower_bound) && Less(value, upper_bound);
    }

    bool IsSubsumedBy(DFConstraint const& other) const {
        return model::GreaterOrEqual(lower_bound, other.lower_bound) &&
               model::LessOrEqual(upper_bound, other.upper_bound);
    }

    bool LongerThan(DFConstraint const& other) const {
        double this_length = upper_bound - lower_bound;
        double other_length = other.upper_bound - other.lower_bound;
        return model::Greater(this_length, other_length) ||
               (model::IsEqual(this_length, other_length) &&
                model::Greater(lower_bound, other.lower_bound));
    }

    std::optional<DFConstraint> IntersectWith(DFConstraint const& other) const {
        double intersect_lower = std::max(lower_bound, other.lower_bound);
        double intersect_upper = std::min(upper_bound, other.upper_bound);

        if (LessOrEqual(intersect_lower, intersect_upper)) {
            return DFConstraint{intersect_lower, intersect_upper};
        }
        return std::nullopt;
    }

    bool IsValid() const {
        return LessOrEqual(lower_bound, upper_bound);
    }

    std::string ToString() const {
        std::stringstream s;
        s << "[" << lower_bound << ", " << upper_bound << "]";
        return s.str();
    }
};

using DF = std::vector<DFConstraint>;

struct DD {
    DF lhs;
    DF rhs;
};

struct DFStringConstraint {
    std::string column_name;
    DFConstraint constraint;

    DFStringConstraint() = default;

    DFStringConstraint(std::string name, DFConstraint constraint)
        : column_name(std::move(name)), constraint(std::move(constraint)) {}

    DFStringConstraint(std::string name, double lower_bound, double upper_bound)
        : column_name(std::move(name)), constraint({lower_bound, upper_bound}) {}

    auto operator<=>(DFStringConstraint const& other) const = default;

    std::string ToString() const {
        std::stringstream s;
        s << column_name << " " << constraint.ToString();
        return s.str();
    }
};

struct DDString {
    std::list<DFStringConstraint> left;
    std::list<DFStringConstraint> right;

    std::string ToString() const {
        return DFToString(left) + " -> " + DFToString(right);
    }

    std::string DFToString(std::list<DFStringConstraint> const& df) const {
        std::stringstream s;
        bool has_constraints = false;
        for (auto constraint : df) {
            if (has_constraints)
                s << " ; ";
            else
                has_constraints = true;
            s << constraint.ToString();
        }
        return s.str();
    }
};

}  // namespace model
