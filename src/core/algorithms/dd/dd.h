#pragma once

#include <list>
#include <sstream>
#include <string>
#include <vector>

namespace model {

struct DFConstraint {
    double lower_bound;
    double upper_bound;

    bool operator==(DFConstraint const& other) const {
        return lower_bound == other.lower_bound && upper_bound == other.upper_bound;
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
               (column_name == other.column_name && lower_bound < other.lower_bound) ||
               (column_name == other.column_name && lower_bound == other.lower_bound &&
                upper_bound < other.upper_bound);
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
