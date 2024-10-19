#pragma once

#include <list>
#include <sstream>
#include <string>
#include <vector>

#include "model/types/builtin.h"
#include "model/types/double_type.h"

namespace model {

inline static bool IsEqual(double first_value, double second_value) {
    return boost::math::relative_difference(first_value, second_value) <
           DoubleType::kDefaultEpsCount * std::numeric_limits<model::Double>::epsilon();
}

inline static bool Less(double first_value, double second_value) {
    return first_value < second_value && !IsEqual(first_value, second_value);
}

inline static bool Greater(double first_value, double second_value) {
    return first_value > second_value && !IsEqual(first_value, second_value);
}

inline static bool LessOrEqual(double first_value, double second_value) {
    return first_value < second_value || IsEqual(first_value, second_value);
}

inline static bool GreaterOrEqual(double first_value, double second_value) {
    return first_value > second_value || IsEqual(first_value, second_value);
}

struct DFConstraint {
    double lower_bound;
    double upper_bound;

    bool operator==(DFConstraint const& other) const {
        return IsEqual(lower_bound, other.lower_bound) && IsEqual(upper_bound, other.upper_bound);
    }

    bool operator!=(DFConstraint const& other) const {
        return !(*this == other);
    }
};

using DF = std::vector<DFConstraint>;

struct DD {
    DF lhs;
    DF rhs;
};

struct DFStringConstraint {
    std::string column_name;
    double lower_bound;
    double upper_bound;

    DFStringConstraint() = default;

    DFStringConstraint(std::string name, double lower, double upper)
        : column_name(name), lower_bound(lower), upper_bound(upper) {}

    bool operator<(DFStringConstraint const& other) const {
        return column_name < other.column_name ||
               (column_name == other.column_name && Less(lower_bound, other.lower_bound)) ||
               (column_name == other.column_name && IsEqual(lower_bound, other.lower_bound) &&
                Less(upper_bound, other.upper_bound));
    }

    bool operator==(DFStringConstraint const& other) const {
        return column_name == other.column_name && IsEqual(lower_bound, other.lower_bound) &&
               IsEqual(upper_bound, other.upper_bound);
    }

    bool operator!=(DFStringConstraint const& other) const {
        return !(*this == other);
    }

    std::string ToString() const {
        std::stringstream s;
        s << column_name << " [" << lower_bound << ", " << upper_bound << "]";
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
